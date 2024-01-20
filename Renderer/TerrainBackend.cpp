#include "Renderer/TerrainBackend.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/TerrainTextureAtlas.h"
#include "Renderer/RenderEngine.h"

#include "Tools/Assert.h"

#include <iostream>

namespace Renderer {

	TerrainBackend::TerrainBackend(
		TerrainRenderer* renderer,
		TerrainSetting& terrainSetting,
		std::vector<TerrainLodDescriptor>& terrainLodDescriptors,
		std::vector<TerrainNodeDescriptor>& terrainNodeDescriptors,
		std::vector<TerrainNodeRuntimeState>& terrainNodeRuntimeStates)
	: mRenderer(renderer)
	, mTerrainSetting(terrainSetting)
	, mTerrainLodDescriptors(terrainLodDescriptors)
	, mTerrainNodeDescriptors(terrainNodeDescriptors) 
	, mTerrainNodeRuntimeStates(terrainNodeRuntimeStates) {

		mHasPreloaded = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mHasPreloaded != nullptr, "CreateEvent Failed");

		CreateGraphicsObject();
	}

	TerrainBackend::~TerrainBackend() {

	}

	void TerrainBackend::Preload() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		auto* terrainTextureAtlasTileCache = mRenderer->mFarTerrainTextureAtlasTileCache.get();
		
		// 录入预加载地形数据的命令
		/*
		auto preloadFarTerrainTextureAtlas = [&](TerrainTextureAtlas* terrainTextureAtlas, TerrainNodeRequestTask& requestTask, GHL::DirectStorageQueue* dstorageQueue) {
			const auto& reTextureFileFormat = terrainTextureAtlas->GetReTextureFileFormat();
			const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
			const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();

			auto* dsFileHandle = terrainTextureAtlas->GetDStorageFile();
			auto& textureAtlas = terrainTextureAtlas->GetTextureAtlas();

			// 当前地形节点对应的TileInfo
			const auto& tileDataInfo = reTileDataInfos.at(requestTask.nextTerrainNodeIndex);

			// 计算TextureRegion
			auto* atlasNode = requestTask.atlasNode;
			D3D12_BOX region{};
			region.front = 0u;
			region.back = 1u;	// 描述深度为1
			region.left = atlasNode->tilePos.x * reTextureFileHeader.tileWidth;
			region.top = atlasNode->tilePos.y * reTextureFileHeader.tileHeight;
			region.right = region.left + reTextureFileHeader.tileWidth;
			region.bottom = region.top + reTextureFileHeader.tileHeight;

			DSTORAGE_REQUEST dsRequest{};
			dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
			dsRequest.Options.CompressionFormat = (DSTORAGE_COMPRESSION_FORMAT)reTextureFileHeader.compressionFormat;
			dsRequest.Destination.Texture.Resource = textureAtlas.Get()->D3DResource();
			dsRequest.Destination.Texture.Region = region;
			dsRequest.Source.File.Source = dsFileHandle->GetDStorageFile();
			dsRequest.Source.File.Offset = tileDataInfo.offset;
			dsRequest.Source.File.Size = tileDataInfo.numBytes;
			dsRequest.UncompressedSize = reTextureFileHeader.tileSlicePitch;

			dstorageQueue->EnqueueRequest(&dsRequest);
		};
		*/

		auto preloadFarTerrainTextureAtlas = [&](TerrainTextureAtlas* terrainTextureAtlas, TerrainNodeRequestTask& requestTask, CommandListWrap& copyCommandList, LinearBufferAllocator* tempLinearBufferAllocator) {
			const auto& reTextureFileFormat = terrainTextureAtlas->GetReTextureFileFormat();
			const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
			const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();
			size_t tileRowPitch = GHL::GetFormatStride(((DXGI_FORMAT)reTextureFileHeader.dxgiFormat)) * reTextureFileHeader.tileWidth;

			auto& fileStreamer = terrainTextureAtlas->GetFileStreamer();
			auto& textureAtlas = terrainTextureAtlas->GetTextureAtlas();
			auto& fileHandle = terrainTextureAtlas->GetFileHandle();

			// 当前地形节点对应的TileInfo
			const auto& currTileDataInfo = reTileDataInfos.at(requestTask.nextTerrainNodeIndex);

			// 计算TextureRegion
			auto* atlasNode = requestTask.atlasNode;
			D3D12_BOX region{};
			region.front = 0u;
			region.back = 1u;	// 描述深度为1
			region.left = atlasNode->tilePos.x * reTextureFileHeader.tileWidth;
			region.top = atlasNode->tilePos.y * reTextureFileHeader.tileHeight;
			region.right = region.left + reTextureFileHeader.tileWidth;
			region.bottom = region.top + reTextureFileHeader.tileHeight;

			// 将文件数据读取到tempLinearBuffer中
			auto dyAlloc = tempLinearBufferAllocator->Allocate(currTileDataInfo.numBytes);
			fileStreamer.seekg(currTileDataInfo.offset, std::ios::beg);
			fileStreamer.read((char*)dyAlloc.cpuAddress, currTileDataInfo.numBytes);

			// CopyBufferRegion
			D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{ dyAlloc.offset,
				{ (DXGI_FORMAT)reTextureFileHeader.dxgiFormat, reTextureFileHeader.tileWidth, reTextureFileHeader.tileHeight, 1u, tileRowPitch} };

			D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(dyAlloc.backResource, layout);
			D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(textureAtlas.Get()->D3DResource(), 0u);
			copyCommandList->D3DCommandList()->CopyTextureRegion(&dstLocation, region.left, region.top, 0, &srcLocation, nullptr);
		};

		// 压入新的CopyFrame
		{
			mBackCopyFence->IncrementExpectedValue();
			mBackCopyFrameTracker->PushCurrentFrame(mBackCopyFence->ExpectedValue());
		}

		std::vector<TerrainNodeRequestTask> terrainNodeRequestTasks;
		std::vector<GpuUpdateTerrainNodeDescriptorRequest> updateTerrainNodeDescriptorRequests;
		// 录制并提交DStorage命令
		{
			auto copyCommandList = mBackCopyCommandListAllocator->AllocateCopyCommandList();
			uint32_t maxLod = mTerrainSetting.smMaxLOD;
			const auto& maxLodDescriptor = mTerrainLodDescriptors.at(maxLod);
			uint32_t nodeStartOffset = maxLodDescriptor.nodeStartOffset;
			uint32_t nodeCount = maxLodDescriptor.nodeCount;

			for (uint32_t i = 0; i < nodeCount; i++) {
				uint32_t currNodeIndex = nodeStartOffset + i;

				// 创建对应的地形请求任务
				TerrainNodeRequestTask requestTask{};
				requestTask.nextTerrainNodeIndex = currNodeIndex;
				auto* atlasNode = terrainTextureAtlasTileCache->GetHead();
				terrainTextureAtlasTileCache->Remove(atlasNode);

				if (atlasNode->terrainNodeIndex != -1) {
					// 实际不会进入此判断
					requestTask.prevTerrainNodeIndex = atlasNode->terrainNodeIndex;
				}
				requestTask.atlasNode = atlasNode;
				terrainNodeRequestTasks.push_back(requestTask);

				// 创建更新请求
				GpuUpdateTerrainNodeDescriptorRequest updateTerrainNodeDescriptorRequest{};
				updateTerrainNodeDescriptorRequest.dstTerrainNodeIndex = requestTask.nextTerrainNodeIndex;
				updateTerrainNodeDescriptorRequest.tilePosX = requestTask.atlasNode->tilePos.x;
				updateTerrainNodeDescriptorRequest.tilePosY = requestTask.atlasNode->tilePos.y;
				updateTerrainNodeDescriptorRequests.push_back(updateTerrainNodeDescriptorRequest);

				// preloadFarTerrainTextureAtlas(mRenderer->mFarTerrainHeightMapAtlas.get(), requestTask, mBackDStorageQueue.get());
				preloadFarTerrainTextureAtlas(mRenderer->mFarTerrainHeightMapAtlas.get(), requestTask, copyCommandList, mBackCopyLinearBufferAllocator.get());

				auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.nextTerrainNodeIndex);
				currNodeRuntimeState.SetInLoading();
			}

			/*
			mBackDStorageFence->IncrementExpectedValue();
			mBackDStorageQueue->EnqueueSignal(*mBackDStorageFence.get());
			mBackDStorageQueue->Submit();
			*/

			copyCommandList->Close();
			mBackCopyQueue->ExecuteCommandList(copyCommandList->D3DCommandList());
			mBackCopyFence->IncrementExpectedValue();
			mBackCopyQueue->SignalFence(*mBackCopyFence.get());
		}

		// 压入新的ComputeFrame
		{
			mBackComputeFence->IncrementExpectedValue();
			mBackComputeFrameTracker->PushCurrentFrame(mBackComputeFence->ExpectedValue());
		}

		{
			// mBackComputeQueue->WaitFence(*mBackDStorageFence.get());
			mBackComputeQueue->WaitFence(*mBackCopyFence.get());

			auto commandList = mBackComputeCommandListAllocator->AllocateComputeCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mBackResourceStateTracker.get(), mBackComputeLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdatePageTableTexturePass");

			// Pass Data
			mUpdateTerrainNodeDescriptorPassData.terrainNodeDescriptorBufferIndex = mRenderer->mTerrainNodeDescriptorBuffer->GetUADescriptor()->GetHeapIndex();
			mUpdateTerrainNodeDescriptorPassData.updateTerrainNodeDescriptorRequestBufferIndex = mUpdateTerrainNodeDescriptorRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			auto passDataAlloc = mBackComputeLinearBufferAllocator->Allocate(sizeof(UpdateTerrainNodeDescriptorPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateTerrainNodeDescriptorPassData, sizeof(UpdateTerrainNodeDescriptorPassData));

			// Upload Data
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch =  resourceStateTracker->TransitionImmediately(mRenderer->mTerrainNodeDescriptorBuffer, GHL::EResourceState::CopyDestination);
			barrierBatch += resourceStateTracker->TransitionImmediately(mRenderer->mTerrainLodDescriptorBuffer, GHL::EResourceState::CopyDestination);
			barrierBatch += mBackResourceStateTracker->TransitionImmediately(mUpdateTerrainNodeDescriptorRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mRenderer->mTerrainNodeDescriptorBuffer, 0u, mTerrainNodeDescriptors.data(), mTerrainNodeDescriptors.size() * sizeof(TerrainNodeDescriptor));
			commandBuffer.UploadBufferRegion(mRenderer->mTerrainLodDescriptorBuffer, 0u, mTerrainLodDescriptors.data(), mTerrainLodDescriptors.size() * sizeof(TerrainLodDescriptor));
			commandBuffer.UploadBufferRegion(mUpdateTerrainNodeDescriptorRequestBuffer, 0u, updateTerrainNodeDescriptorRequests.data(), updateTerrainNodeDescriptorRequests.size() * sizeof(GpuUpdateTerrainNodeDescriptorRequest));

			barrierBatch =  resourceStateTracker->TransitionImmediately(mRenderer->mTerrainNodeDescriptorBuffer, GHL::EResourceState::UnorderedAccess);
			barrierBatch += resourceStateTracker->TransitionImmediately(mRenderer->mTerrainLodDescriptorBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += mBackResourceStateTracker->TransitionImmediately(mUpdateTerrainNodeDescriptorRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetComputeRootSignature();
			commandBuffer.SetComputePipelineState(smUpdateTerrainNodeDescriptorSN);
			commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);

			uint32_t threadGroupCountX = (updateTerrainNodeDescriptorRequests.size() + smThreadSizeInGroup - 1) / smThreadSizeInGroup;
			commandBuffer.Dispatch(threadGroupCountX, 1u, 1u);

			commandBuffer.PIXEndEvent();
			commandList->Close();
			mBackComputeQueue->ExecuteCommandList(commandList->D3DCommandList());
			mBackComputeQueue->SignalFence(*mBackComputeFence.get());
		}

		// 等待CopyFrame与ComputeFrame完成，然后处理之
		{
			mBackCopyFence->Wait();
			mBackComputeFence->Wait();
			mBackCopyFrameTracker->PopCompletedFrame(mBackCopyFence->CompletedValue());
			mBackComputeFrameTracker->PopCompletedFrame(mBackComputeFence->CompletedValue());
		}

		// GPU任务完成后，更新CPU中节点资源驻留状态
		for (const auto& requestTask : terrainNodeRequestTasks) {
			auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.nextTerrainNodeIndex);
			currNodeRuntimeState.SetInTexture();
			currNodeRuntimeState.atlasNode = requestTask.atlasNode;
			
			requestTask.atlasNode->terrainNodeIndex = requestTask.nextTerrainNodeIndex;
			terrainTextureAtlasTileCache->AddTail(requestTask.atlasNode);
		}

		// 加载完成后通知后台线程，启动资源调度线程
		::SetEvent(mHasPreloaded);
	}

	// 后台线程
	void TerrainBackend::BackendThread() {
		// 等待预先加载完成后启动任务
		::WaitForSingleObject(mHasPreloaded, INFINITE);

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* mainRenderFrameFence = renderEngine->mRenderFrameFence.get();
		auto* pipelineResourceStorage = renderEngine->mPipelineResourceStorage;
		uint64_t previousMainFrameFenceCompletedValue = 0u;

		while (mThreadRunning) {
			// 资源加载也会占用GPU资源，因此我们尽量一帧更新一次，这里选择在一个主渲染帧在GPU上的任务完成后进行命令的录制
			uint64_t currentMainFrameFenceCompletedValue = mainRenderFrameFence->CompletedValue();

			// 新的主渲染帧完成
			if (previousMainFrameFenceCompletedValue != currentMainFrameFenceCompletedValue) {
				previousMainFrameFenceCompletedValue = previousMainFrameFenceCompletedValue;

				// 压入新的CopyFrame与ComputeFrame
				uint64_t currFrameCopyFenceExpectedValue = mBackCopyFence->IncrementExpectedValue();
				uint64_t currFrameComputeFenceExpectedValue = mBackComputeFence->IncrementExpectedValue();

				mBackCopyFrameTracker->PushCurrentFrame(currFrameCopyFenceExpectedValue);
				mBackComputeFrameTracker->PushCurrentFrame(currFrameComputeFenceExpectedValue);

				// 根据当前摄像机所处的位置计算需要更新的地形节点
				auto cameraPosition = pipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.position;
				std::vector<TerrainNodeRequestTask> terrainNodeRequestTasks;
				ProcessTerrainNodeRequest(terrainNodeRequestTasks, cameraPosition);

				// 记录当前帧对应的Gpu命令
				RecordedGpuCommand recordedGpuCommand{};
				recordedGpuCommand.copyFence = mBackCopyFence.get();
				recordedGpuCommand.copyQueue = mBackCopyQueue.get();
				recordedGpuCommand.copyCommandList = nullptr;
				recordedGpuCommand.copyFenceExpectedValue = currFrameCopyFenceExpectedValue;
				recordedGpuCommand.computeQueue = mBackComputeQueue.get();
				recordedGpuCommand.computeFence = mBackComputeFence.get();
				recordedGpuCommand.computeCommandList = nullptr;
				recordedGpuCommand.computeFenceExpectedValue = currFrameComputeFenceExpectedValue;

				// 压入队列中
				mRecordedGpuCommands.Push(std::move(recordedGpuCommand));
			}
		}
	}

	void TerrainBackend::ProcessTerrainNodeRequest(std::vector<TerrainNodeRequestTask>& requestTasks, Math::Vector3 cameraPosition) {
		auto* tileCache = mRenderer->mFarTerrainTextureAtlasTileCache.get();

		auto GetFixedPos = [](const Math::Vector3& pos, float nodeMeterSize) {
			return Math::Int2{
				(int)std::floor(pos.x / nodeMeterSize + 0.5f) * (int)nodeMeterSize,
				(int)std::floor(pos.z / nodeMeterSize + 0.5f) * (int)nodeMeterSize
			};
		};

		// 处理优先级: LOD0 LOD1 ... LOD4
		for (int32_t currLod = 0; currLod <= mTerrainSetting.smMaxLOD; currLod++) {
			// 最高级别的节点持久化驻留
			if (currLod == mTerrainSetting.smMaxLOD) {
				const auto& maxLodDescriptor = mTerrainLodDescriptors.at(currLod);
				uint32_t nodeStartOffset = maxLodDescriptor.nodeStartOffset;
				uint32_t nodeCount = maxLodDescriptor.nodeCount;

				// 更新LOD4中节点在LRUCache中的位置
				for (uint32_t i = 0; i < nodeCount; i++) {
					uint32_t currNodeIndex = nodeStartOffset + i;
					auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(currNodeIndex);
					ASSERT_FORMAT(currNodeRuntimeState.inTexture == true);
					tileCache->Remove(currNodeRuntimeState.atlasNode);
					tileCache->AddTail(currNodeRuntimeState.atlasNode);
				}
				continue;
			}

			// 其他节点做额外考虑
			const auto& currLodDescriptor = mTerrainLodDescriptors.at(currLod);
			Math::Int2 fixedPos = GetFixedPos(cameraPosition, (float)currLodDescriptor.nodeMeterSize);
			// 考虑范围为： -smsmTerrainDataRange, smTerrainDataRange 内的地形节点
			for (int32_t yIndex = -mTerrainSetting.smTerrainDataLoadedRange; yIndex <= mTerrainSetting.smTerrainDataLoadedRange; yIndex++) {
				for (int32_t xIndex = -mTerrainSetting.smTerrainDataLoadedRange; xIndex <= mTerrainSetting.smTerrainDataLoadedRange; xIndex++) {
					int32_t currNodeLocationX = fixedPos.x + xIndex;
					int32_t currNodeLocationY = fixedPos.y + yIndex;

					// 计算出当前节点的全局索引
					uint32_t nodeCountPerRow = mTerrainSetting.smTerrainMeterSize/ currLodDescriptor.nodeMeterSize;
					uint32_t currNodeIndex = currLodDescriptor.nodeStartOffset + currNodeLocationY * nodeCountPerRow + currNodeLocationX;

					// 获取该节点的实时状态
					auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(currNodeIndex);
					if (currNodeRuntimeState.inQueue || currNodeRuntimeState.inLoading) { 
						continue; 
					}
					else if (currNodeRuntimeState.inTexture) {
						tileCache->Remove(currNodeRuntimeState.atlasNode);
						tileCache->AddTail(currNodeRuntimeState.atlasNode);
					}
					else {
						// 创建对应的地形请求任务(但不分配图集元素)
						TerrainNodeRequestTask requestTask{};
						requestTask.nextTerrainNodeIndex = currNodeIndex;
						requestTasks.push_back(requestTask);
					}
				}
			}
		}

		// 将requestTask翻转，使得LOD级别更高的节点出现在队列前面
		std::reverse(requestTasks.begin(), requestTasks.end());

		// 保留一定数量的请求任务，一次只加载smTerrainDataLoadedLimit个地形节点的数据
		if (requestTasks.size() > mTerrainSetting.smTerrainDataLoadedLimit) {
			requestTasks.resize(mTerrainSetting.smTerrainDataLoadedLimit);
		}
	}

	void TerrainBackend::CreateGraphicsObject() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageFactory = renderEngine->mDStorageFactory.get();

		auto  shaderPath = renderEngine->smEngineShaderPath;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();

		// 创建DStorageAPI对象
		{
			/*
			mBackDStorageQueue = std::make_unique<GHL::DirectStorageQueue>(device, dstorageFactory, DSTORAGE_REQUEST_SOURCE_FILE);
			mBackDStorageFence = std::make_unique<GHL::Fence>(device);
			mBackDStorageFence->SetDebugName("TerrainBackend_DStorageFence");
			*/
		}

		// 创建图形API对象并设置回调函数
		{
			mBackResourceStateTracker = std::make_unique<Renderer::ResourceStateTracker>();

			mBackCopyQueue = std::make_unique<GHL::CopyQueue>(device);
			mBackCopyQueue->SetDebugName("TerrainBackend_CopyQueue");
			mBackCopyFence = std::make_unique<GHL::Fence>(device);
			mBackCopyFence->SetDebugName("TerrainBackend_CopyFence");
			mBackCopyFrameTracker = std::make_unique<Renderer::RingFrameTracker>(smMaxBackFrameCount);
			mBackCopyLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mBackCopyFrameTracker.get(), 0x1000000);	// 16MB
			mBackCopyCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mBackCopyFrameTracker.get());

			mBackComputeQueue = std::make_unique<GHL::ComputeQueue>(device);
			mBackComputeQueue->SetDebugName("TerrainBackend_ComputeQueue");
			mBackComputeFence = std::make_unique<GHL::Fence>(device);
			mBackComputeFence->SetDebugName("TerrainBackend_ComputeFence");
			mBackComputeFrameTracker = std::make_unique<Renderer::RingFrameTracker>(smMaxBackFrameCount);
			mBackComputeLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mBackCopyFrameTracker.get());
			mBackComputeCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mBackComputeFrameTracker.get());
		}

		// 创建图形对象
		{
			auto* terrainHeightMapAtlas = mRenderer->mFarTerrainHeightMapAtlas.get();

			Renderer::BufferDesc _UpdateTerrainNodeDescriptorRequestBufferDesc{};
			_UpdateTerrainNodeDescriptorRequestBufferDesc.stride = sizeof(GpuUpdateTerrainNodeDescriptorRequest);
			_UpdateTerrainNodeDescriptorRequestBufferDesc.size = _UpdateTerrainNodeDescriptorRequestBufferDesc.stride * terrainHeightMapAtlas->GetTileCount();
			_UpdateTerrainNodeDescriptorRequestBufferDesc.usage = GHL::EResourceUsage::Default;
			_UpdateTerrainNodeDescriptorRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_UpdateTerrainNodeDescriptorRequestBufferDesc.initialState = GHL::EResourceState::Common;
			_UpdateTerrainNodeDescriptorRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mUpdateTerrainNodeDescriptorRequestBuffer = resourceAllocator->Allocate(device, _UpdateTerrainNodeDescriptorRequestBufferDesc, descriptorAllocator, nullptr);

			mBackResourceStateTracker->StartTracking(mUpdateTerrainNodeDescriptorRequestBuffer);
		}

		// 创建着色器程序
		{
			shaderManger->CreateComputeShader(smUpdateTerrainNodeDescriptorSN,
				[&](ComputeStateProxy& proxy) {
					proxy.csFilepath = shaderPath + "TerrainRenderer/TerrainNodeDescriptorUpdater.hlsl";
				}
			);
		}
	}

}