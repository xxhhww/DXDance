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
		
		// ¼��Ԥ���ص������ݵ�����
		/*
		auto preloadFarTerrainTextureAtlas = [&](TerrainTextureAtlas* terrainTextureAtlas, TerrainNodeRequestTask& requestTask, GHL::DirectStorageQueue* dstorageQueue) {
			const auto& reTextureFileFormat = terrainTextureAtlas->GetReTextureFileFormat();
			const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
			const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();

			auto* dsFileHandle = terrainTextureAtlas->GetDStorageFile();
			auto& textureAtlas = terrainTextureAtlas->GetTextureAtlas();

			// ��ǰ���νڵ��Ӧ��TileInfo
			const auto& tileDataInfo = reTileDataInfos.at(requestTask.nextTerrainNodeIndex);

			// ����TextureRegion
			auto* atlasNode = requestTask.atlasNode;
			D3D12_BOX region{};
			region.front = 0u;
			region.back = 1u;	// �������Ϊ1
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

			// ��ǰ���νڵ��Ӧ��TileInfo
			const auto& currTileDataInfo = reTileDataInfos.at(requestTask.nextTerrainNodeIndex);

			// ����TextureRegion
			auto* atlasNode = requestTask.atlasNode;
			D3D12_BOX region{};
			region.front = 0u;
			region.back = 1u;	// �������Ϊ1
			region.left = atlasNode->tilePos.x * reTextureFileHeader.tileWidth;
			region.top = atlasNode->tilePos.y * reTextureFileHeader.tileHeight;
			region.right = region.left + reTextureFileHeader.tileWidth;
			region.bottom = region.top + reTextureFileHeader.tileHeight;

			// ���ļ����ݶ�ȡ��tempLinearBuffer��
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

		// ѹ���µ�CopyFrame
		{
			mBackCopyFence->IncrementExpectedValue();
			mBackCopyFrameTracker->PushCurrentFrame(mBackCopyFence->ExpectedValue());
		}

		std::vector<TerrainNodeRequestTask> terrainNodeRequestTasks;
		std::vector<GpuUpdateTerrainNodeDescriptorRequest> updateTerrainNodeDescriptorRequests;
		// ¼�Ʋ��ύDStorage����
		{
			auto copyCommandList = mBackCopyCommandListAllocator->AllocateCopyCommandList();
			uint32_t maxLod = mTerrainSetting.smMaxLOD;
			const auto& maxLodDescriptor = mTerrainLodDescriptors.at(maxLod);
			uint32_t nodeStartOffset = maxLodDescriptor.nodeStartOffset;
			uint32_t nodeCount = maxLodDescriptor.nodeCount;

			for (uint32_t i = 0; i < nodeCount; i++) {
				uint32_t currNodeIndex = nodeStartOffset + i;

				// ������Ӧ�ĵ�����������
				TerrainNodeRequestTask requestTask{};
				requestTask.nextTerrainNodeIndex = currNodeIndex;
				auto* atlasNode = terrainTextureAtlasTileCache->GetHead();
				terrainTextureAtlasTileCache->Remove(atlasNode);

				if (atlasNode->terrainNodeIndex != -1) {
					// ʵ�ʲ��������ж�
					requestTask.prevTerrainNodeIndex = atlasNode->terrainNodeIndex;
				}
				requestTask.atlasNode = atlasNode;
				terrainNodeRequestTasks.push_back(requestTask);

				// ������������
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

		// ѹ���µ�ComputeFrame
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

		// �ȴ�CopyFrame��ComputeFrame��ɣ�Ȼ����֮
		{
			mBackCopyFence->Wait();
			mBackComputeFence->Wait();
			mBackCopyFrameTracker->PopCompletedFrame(mBackCopyFence->CompletedValue());
			mBackComputeFrameTracker->PopCompletedFrame(mBackComputeFence->CompletedValue());
		}

		// GPU������ɺ󣬸���CPU�нڵ���Դפ��״̬
		for (const auto& requestTask : terrainNodeRequestTasks) {
			auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.nextTerrainNodeIndex);
			currNodeRuntimeState.SetInTexture();
			currNodeRuntimeState.atlasNode = requestTask.atlasNode;
			
			requestTask.atlasNode->terrainNodeIndex = requestTask.nextTerrainNodeIndex;
			terrainTextureAtlasTileCache->AddTail(requestTask.atlasNode);
		}

		// ������ɺ�֪ͨ��̨�̣߳�������Դ�����߳�
		::SetEvent(mHasPreloaded);
	}

	// ��̨�߳�
	void TerrainBackend::BackendThread() {
		// �ȴ�Ԥ�ȼ�����ɺ���������
		::WaitForSingleObject(mHasPreloaded, INFINITE);

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* mainRenderFrameFence = renderEngine->mRenderFrameFence.get();
		auto* pipelineResourceStorage = renderEngine->mPipelineResourceStorage;
		uint64_t previousMainFrameFenceCompletedValue = 0u;

		while (mThreadRunning) {
			// ��Դ����Ҳ��ռ��GPU��Դ��������Ǿ���һ֡����һ�Σ�����ѡ����һ������Ⱦ֡��GPU�ϵ�������ɺ���������¼��
			uint64_t currentMainFrameFenceCompletedValue = mainRenderFrameFence->CompletedValue();

			// �µ�����Ⱦ֡���
			if (previousMainFrameFenceCompletedValue != currentMainFrameFenceCompletedValue) {
				previousMainFrameFenceCompletedValue = previousMainFrameFenceCompletedValue;

				// ѹ���µ�CopyFrame��ComputeFrame
				uint64_t currFrameCopyFenceExpectedValue = mBackCopyFence->IncrementExpectedValue();
				uint64_t currFrameComputeFenceExpectedValue = mBackComputeFence->IncrementExpectedValue();

				mBackCopyFrameTracker->PushCurrentFrame(currFrameCopyFenceExpectedValue);
				mBackComputeFrameTracker->PushCurrentFrame(currFrameComputeFenceExpectedValue);

				// ���ݵ�ǰ�����������λ�ü�����Ҫ���µĵ��νڵ�
				auto cameraPosition = pipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.position;
				std::vector<TerrainNodeRequestTask> terrainNodeRequestTasks;
				ProcessTerrainNodeRequest(terrainNodeRequestTasks, cameraPosition);

				// ��¼��ǰ֡��Ӧ��Gpu����
				RecordedGpuCommand recordedGpuCommand{};
				recordedGpuCommand.copyFence = mBackCopyFence.get();
				recordedGpuCommand.copyQueue = mBackCopyQueue.get();
				recordedGpuCommand.copyCommandList = nullptr;
				recordedGpuCommand.copyFenceExpectedValue = currFrameCopyFenceExpectedValue;
				recordedGpuCommand.computeQueue = mBackComputeQueue.get();
				recordedGpuCommand.computeFence = mBackComputeFence.get();
				recordedGpuCommand.computeCommandList = nullptr;
				recordedGpuCommand.computeFenceExpectedValue = currFrameComputeFenceExpectedValue;

				// ѹ�������
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

		// �������ȼ�: LOD0 LOD1 ... LOD4
		for (int32_t currLod = 0; currLod <= mTerrainSetting.smMaxLOD; currLod++) {
			// ��߼���Ľڵ�־û�פ��
			if (currLod == mTerrainSetting.smMaxLOD) {
				const auto& maxLodDescriptor = mTerrainLodDescriptors.at(currLod);
				uint32_t nodeStartOffset = maxLodDescriptor.nodeStartOffset;
				uint32_t nodeCount = maxLodDescriptor.nodeCount;

				// ����LOD4�нڵ���LRUCache�е�λ��
				for (uint32_t i = 0; i < nodeCount; i++) {
					uint32_t currNodeIndex = nodeStartOffset + i;
					auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(currNodeIndex);
					ASSERT_FORMAT(currNodeRuntimeState.inTexture == true);
					tileCache->Remove(currNodeRuntimeState.atlasNode);
					tileCache->AddTail(currNodeRuntimeState.atlasNode);
				}
				continue;
			}

			// �����ڵ������⿼��
			const auto& currLodDescriptor = mTerrainLodDescriptors.at(currLod);
			Math::Int2 fixedPos = GetFixedPos(cameraPosition, (float)currLodDescriptor.nodeMeterSize);
			// ���Ƿ�ΧΪ�� -smsmTerrainDataRange, smTerrainDataRange �ڵĵ��νڵ�
			for (int32_t yIndex = -mTerrainSetting.smTerrainDataLoadedRange; yIndex <= mTerrainSetting.smTerrainDataLoadedRange; yIndex++) {
				for (int32_t xIndex = -mTerrainSetting.smTerrainDataLoadedRange; xIndex <= mTerrainSetting.smTerrainDataLoadedRange; xIndex++) {
					int32_t currNodeLocationX = fixedPos.x + xIndex;
					int32_t currNodeLocationY = fixedPos.y + yIndex;

					// �������ǰ�ڵ��ȫ������
					uint32_t nodeCountPerRow = mTerrainSetting.smTerrainMeterSize/ currLodDescriptor.nodeMeterSize;
					uint32_t currNodeIndex = currLodDescriptor.nodeStartOffset + currNodeLocationY * nodeCountPerRow + currNodeLocationX;

					// ��ȡ�ýڵ��ʵʱ״̬
					auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(currNodeIndex);
					if (currNodeRuntimeState.inQueue || currNodeRuntimeState.inLoading) { 
						continue; 
					}
					else if (currNodeRuntimeState.inTexture) {
						tileCache->Remove(currNodeRuntimeState.atlasNode);
						tileCache->AddTail(currNodeRuntimeState.atlasNode);
					}
					else {
						// ������Ӧ�ĵ�����������(��������ͼ��Ԫ��)
						TerrainNodeRequestTask requestTask{};
						requestTask.nextTerrainNodeIndex = currNodeIndex;
						requestTasks.push_back(requestTask);
					}
				}
			}
		}

		// ��requestTask��ת��ʹ��LOD������ߵĽڵ�����ڶ���ǰ��
		std::reverse(requestTasks.begin(), requestTasks.end());

		// ����һ����������������һ��ֻ����smTerrainDataLoadedLimit�����νڵ������
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

		// ����DStorageAPI����
		{
			/*
			mBackDStorageQueue = std::make_unique<GHL::DirectStorageQueue>(device, dstorageFactory, DSTORAGE_REQUEST_SOURCE_FILE);
			mBackDStorageFence = std::make_unique<GHL::Fence>(device);
			mBackDStorageFence->SetDebugName("TerrainBackend_DStorageFence");
			*/
		}

		// ����ͼ��API�������ûص�����
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

		// ����ͼ�ζ���
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

		// ������ɫ������
		{
			shaderManger->CreateComputeShader(smUpdateTerrainNodeDescriptorSN,
				[&](ComputeStateProxy& proxy) {
					proxy.csFilepath = shaderPath + "TerrainRenderer/TerrainNodeDescriptorUpdater.hlsl";
				}
			);
		}
	}

}