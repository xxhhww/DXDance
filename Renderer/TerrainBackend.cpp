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
		auto preloadFarTerrainTextureAtlas = [&](TerrainTextureAtlas* terrainTextureAtlas, TerrainNodeRequestTask& requestTask, GHL::DirectStorageQueue* dstorageQueue) {
			const auto& reTextureFileFormat = terrainTextureAtlas->GetReTextureFileFormat();
			const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
			const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();

			auto* dsFileHandle = terrainTextureAtlas->GetDStorageFile();
			auto& textureAtlas = terrainTextureAtlas->GetTextureAtlas();
			const auto& uncompressedSize = terrainTextureAtlas->GetUncompressedSize();

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
			dsRequest.Options.CompressionFormat = (DSTORAGE_COMPRESSION_FORMAT)1;
			dsRequest.Destination.Texture.Resource = textureAtlas.Get()->D3DResource();
			dsRequest.Destination.Texture.Region = region;
			dsRequest.Source.File.Source = dsFileHandle->GetDStorageFile();
			dsRequest.Source.File.Offset = tileDataInfo.offset;
			dsRequest.Source.File.Size = tileDataInfo.numBytes;
			dsRequest.UncompressedSize = reTextureFileHeader.tileSlicePitch;

			dstorageQueue->EnqueueRequest(&dsRequest);
		};

		/*
		auto preloadFarTerrainTextureAtlas = [&](TerrainTextureAtlas* terrainTextureAtlas, TerrainNodeRequestTask& requestTask, CommandListWrap& copyCommandList, LinearBufferAllocator* tempLinearBufferAllocator) {
			const auto& reTextureFileFormat = terrainTextureAtlas->GetReTextureFileFormat();
			const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
			const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();
			size_t tileRowPitch = GHL::GetFormatStride(((DXGI_FORMAT)reTextureFileHeader.dxgiFormat)) * reTextureFileHeader.tileWidth;

			auto& fileStreamer = terrainTextureAtlas->GetFileStreamer();
			auto& textureAtlas = terrainTextureAtlas->GetTextureAtlas();
			auto& fileHandle = terrainTextureAtlas->GetFileHandle();
			const auto& uncompressedSize = terrainTextureAtlas->GetUncompressedSize();

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
		*/

		// ѹ���µ�֡
		{
			mBackComputeFence->IncrementExpectedValue();
			mBackFrameTracker->PushCurrentFrame(mBackComputeFence->ExpectedValue());
		}

		std::vector<TerrainNodeRequestTask> terrainNodeRequestTasks;
		std::vector<GpuUpdateTerrainNodeDescriptorRequest> updateTerrainNodeDescriptorRequests;
		// ¼�Ʋ��ύDStorage����
		{
			// auto copyCommandList = mBackPoolCommandListAllocator->AllocateCopyCommandList();
			uint32_t maxLod = mTerrainSetting.maxLOD;
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
				const auto& currTerrainNodeDescriptor = mTerrainNodeDescriptors.at(requestTask.nextTerrainNodeIndex);
				GpuUpdateTerrainNodeDescriptorRequest updateTerrainNodeDescriptorRequest{};
				updateTerrainNodeDescriptorRequest.dstTerrainNodeIndex = requestTask.nextTerrainNodeIndex;
				updateTerrainNodeDescriptorRequest.terrainNodeDescriptor = currTerrainNodeDescriptor;
				updateTerrainNodeDescriptorRequest.terrainNodeDescriptor.tilePosX = requestTask.atlasNode->tilePos.x;
				updateTerrainNodeDescriptorRequest.terrainNodeDescriptor.tilePosY = requestTask.atlasNode->tilePos.y;
				updateTerrainNodeDescriptorRequests.push_back(updateTerrainNodeDescriptorRequest);

				preloadFarTerrainTextureAtlas(mRenderer->mFarTerrainHeightMapAtlas.get(), requestTask, mBackDStorageQueue.get());
				// preloadFarTerrainTextureAtlas(mRenderer->mFarTerrainHeightMapAtlas.get(), requestTask, copyCommandList, mBackTempLinearBufferAllocator.get());

				auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.nextTerrainNodeIndex);
				currNodeRuntimeState.SetInLoading();
			}

			mBackDStorageFence->IncrementExpectedValue();
			mBackDStorageQueue->EnqueueSignal(*mBackDStorageFence.get(), mBackDStorageFence->ExpectedValue());
			mBackDStorageQueue->Submit();

			/*
			copyCommandList->Close();
			mBackCopyQueue->ExecuteCommandList(copyCommandList->D3DCommandList());
			mBackCopyFence->IncrementExpectedValue();
			mBackCopyQueue->SignalFence(*mBackCopyFence.get());
			*/
		}

		// �ύComputeQueue����(��Ҫ�ύ��GPU�ȴ�DStorageFence������)�����½ڵ���Դ״̬
		{
			mBackComputeQueue->WaitFence(*mBackDStorageFence.get());
			// mBackComputeQueue->WaitFence(*mBackCopyFence.get());

			auto commandList = mBackPoolCommandListAllocator->AllocateComputeCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mBackResourceStateTracker.get(), mBackLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdatePageTableTexturePass");

			// Pass Data
			mUpdateTerrainNodeDescriptorPassData.terrainNodeDescriptorBufferIndex = mRenderer->mTerrainNodeDescriptorBuffer->GetUADescriptor()->GetHeapIndex();
			mUpdateTerrainNodeDescriptorPassData.updateTerrainNodeDescriptorRequestBufferIndex = mUpdateTerrainNodeDescriptorRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			auto passDataAlloc = mBackLinearBufferAllocator->Allocate(sizeof(UpdateTerrainNodeDescriptorPassData));
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

		// �ȴ�Compute������ɣ�Ȼ����֮
		{
			mBackComputeFence->Wait();
			mBackFrameTracker->PopCompletedFrame(mBackComputeFence->CompletedValue());
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
		// �ȴ�Ԥ�ȼ������
		::WaitForSingleObject(mHasPreloaded, INFINITE);
		while (mThreadRunning) {

		}
	}

	void TerrainBackend::CreateGraphicsObject() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageFactory = renderEngine->mDStorageFactory.get();

		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();

		// ����DStorageAPI����
		{
			mBackDStorageQueue = std::make_unique<GHL::DirectStorageQueue>(device, dstorageFactory, DSTORAGE_REQUEST_SOURCE_FILE);
			mBackDStorageFence = std::make_unique<GHL::Fence>(device);
			mBackDStorageFence->SetDebugName("TerrainBackend_DStorageFence");
		}

		// ����ͼ��API�������ûص�����
		{
			/*
			mBackCopyQueue = std::make_unique<GHL::CopyQueue>(device);
			mBackCopyQueue->SetDebugName("TerrainBackend_CopyQueue");
			mBackCopyFence = std::make_unique<GHL::Fence>(device);
			mBackCopyFence->SetDebugName("TerrainBackend_CopyFence");
			mBackTempLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mBackFrameTracker.get(), 0x1000000);	// 16MB
			*/

			mBackGrahpicsQueue = std::make_unique<GHL::GraphicsQueue>(device);
			mBackGrahpicsQueue->SetDebugName("TerrainBackend_GraphicsQueue");
			mBackGraphicsFence = std::make_unique<GHL::Fence>(device);
			mBackGraphicsFence->SetDebugName("TerrainBackend_GraphicsFence");
			
			mBackComputeQueue = std::make_unique<GHL::ComputeQueue>(device);
			mBackComputeQueue->SetDebugName("TerrainBackend_ComputeQueue");
			mBackComputeFence = std::make_unique<GHL::Fence>(device);
			mBackComputeFence->SetDebugName("TerrainBackend_ComputeFence");

			mBackFrameTracker = std::make_unique<Renderer::RingFrameTracker>(smMaxBackFrameCount);
			mBackResourceStateTracker = std::make_unique<Renderer::ResourceStateTracker>();
			mBackLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mBackFrameTracker.get());
			mBackPoolCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mBackFrameTracker.get());
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
				[](ComputeStateProxy& proxy) {
					proxy.csFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/TerrainRenderer/UpdateTerrainNodeDescriptor.hlsl";
				}
			);
		}
	}

}