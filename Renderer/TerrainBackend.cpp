#include "Renderer/TerrainBackend.h"
#include "Renderer/TerrainTextureAtlas.h"
#include "Renderer/TerrainTextureArray.h"
#include "Renderer/TerrainTiledTexture.h"
#include "Renderer/RenderEngine.h"

#include "Tools/Assert.h"

#include <iostream>

namespace Renderer {

	/*
	* ������θ��µ��νڵ������Ľṹ��
	*/
	struct GpuUpdateTerrainNodeDescriptorRequest {
	public:
		uint32_t srcTerrainNodeIndex{ 65536u };	// Դ���νڵ�����(65536Ϊ��Чֵ)
		uint32_t dstTerrainNodeIndex;			// �յ��νڵ�����

		uint32_t tilePosX;	// �յ��νڵ�������ͼ���ϵ�����(���srcTerrainNodeIndex������Чֵ���������ԭ������srcTerrainNodeIndex�����ڱ�������)
		uint32_t tilePosY;	// ͬ��
	};

	TerrainBackend::TerrainBackend(
		TerrainRenderer* renderer,
		TerrainSetting& terrainSetting,
		std::vector<TerrainLodDescriptor>& terrainLodDescriptors,
		std::vector<TerrainNodeDescriptor>& terrainNodeDescriptors,
		std::vector<TerrainNodeRuntimeState>& terrainNodeRuntimeStates,
		std::vector<TerrainTiledTextureTileRuntimeState>& terrainTiledTextureTileRuntimeStates)
	: mRenderer(renderer)
	, mTerrainSetting(terrainSetting)
	, mTerrainLodDescriptors(terrainLodDescriptors)
	, mTerrainNodeDescriptors(terrainNodeDescriptors) 
	, mTerrainNodeRuntimeStates(terrainNodeRuntimeStates) 
	, mTerrainTiledTextureTileRuntimeStates(terrainTiledTextureTileRuntimeStates) {
		mReservedTerrainNodeRequestTasks.resize(smMaxBackFrameCount);
		mReservedTerrainTiledTextureTileRequestTasks.resize(smMaxBackFrameCount);
		mFrameCompletedFlags.resize(smMaxBackFrameCount);

		mHasPreloaded = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mHasPreloaded != nullptr, "CreateEvent Failed");

		// ����ͼ�ζ���
		CreateGraphicsObject();

		// �����߳�
		mThread = std::thread([this]() {
				this->BackendThread();
			}
		);
	}

	TerrainBackend::~TerrainBackend() {
		mThreadRunning = false;
	}

	void TerrainBackend::Preload() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceStorage = renderEngine->mPipelineResourceStorage;
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		const auto& cameraPosition = resourceStorage->rootConstantsPerFrame.currentEditorCamera.position;

		auto* terrainTextureAtlasTileCache = mRenderer->mFarTerrainTextureAtlasTileCache.get();
		
		// Ԥ���ص�����������
		auto preloadTerrainTextureArray = [&](TerrainTextureArray* terrainTextureArray, GHL::CommandQueue* mappingQueue, GHL::Fence* mappingFence, GHL::DirectStorageQueue* dstorageQueue, GHL::Fence* dstorageFence) {
			const auto& reTextureFileFormat = terrainTextureArray->GetReTextureFileFormat();
			const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
			const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();

			auto* dsFileHandle = terrainTextureArray->GetDStorageFile();
			auto& textureArray = terrainTextureArray->GetTextureArray();

			uint32_t currTileIndex = 0u;
			for (uint32_t arrayIndex = 0; arrayIndex < reTextureFileHeader.arraySize; arrayIndex++) {
				for (uint32_t mipIndex = 0; mipIndex < reTextureFileHeader.mipLevels; mipIndex++) {
					uint32_t currSubresourceIndex = arrayIndex * reTextureFileHeader.mipLevels + mipIndex;

					// �����mipLevel��Tile�ĸ���
					uint32_t localTileCountPerAxis = (reTextureFileHeader.imageHeight / std::pow(2, mipIndex)) / reTextureFileHeader.tileHeight;
					uint32_t localTileCount = localTileCountPerAxis * localTileCountPerAxis;

					uint32_t tilePosX = 0u;
					uint32_t tilePosY = 0u;
					for (uint32_t localTileIndex = 0u; localTileIndex < localTileCount; localTileIndex++) {
						const auto& tileDataInfo = reTileDataInfos.at(currTileIndex);

						// Mapping
						auto* heapAllocation = mRenderer->mTerrainTextureArrayHeapAllocator->Allocate(reTextureFileHeader.tileSlicePitch);
						uint32_t numTiles = 1u;
						std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags(numTiles, D3D12_TILE_RANGE_FLAG_NONE);
						// if the number of standard (not packed) mips is n, then start updating at subresource n
						D3D12_TILED_RESOURCE_COORDINATE tiledRegionStartCoordinates{ tilePosX, tilePosY, 0, currSubresourceIndex };
						D3D12_TILE_REGION_SIZE tiledRegionSizes{ numTiles, FALSE, 0, 0, 0 };

						// perform packed mip tile mapping on the copy queue
						mappingQueue->D3DCommandQueue()->UpdateTileMappings(
							textureArray->D3DResource(),
							numTiles,
							&tiledRegionStartCoordinates,
							&tiledRegionSizes,
							heapAllocation->heap->D3DHeap(),
							numTiles,
							rangeFlags.data(),
							&heapAllocation->tileOffset,
							nullptr,
							D3D12_TILE_MAPPING_FLAG_NONE
						);
						mappingFence->IncrementExpectedValue();
						mappingQueue->SignalFence(*mappingFence);
						mappingFence->Wait();

						// ����TextureRegion
						D3D12_BOX region{};
						region.front = 0u;
						region.back = 1u;	// �������Ϊ1
						region.left = tilePosX * reTextureFileHeader.tileWidth;
						region.top = tilePosY * reTextureFileHeader.tileHeight;
						region.right = region.left + reTextureFileHeader.tileWidth;
						region.bottom = region.top + reTextureFileHeader.tileHeight;

						tilePosX++;
						if (tilePosX >= localTileCountPerAxis) {
							tilePosX = 0u;
							tilePosY++;
						}

						DSTORAGE_REQUEST dsRequest{};
						dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
						dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TILES;
						dsRequest.Options.CompressionFormat = (DSTORAGE_COMPRESSION_FORMAT)reTextureFileHeader.compressionFormat;
						dsRequest.Destination.Tiles.Resource = textureArray->D3DResource();
						dsRequest.Destination.Tiles.TiledRegionStartCoordinate = tiledRegionStartCoordinates;
						dsRequest.Destination.Tiles.TileRegionSize = tiledRegionSizes;
						dsRequest.Source.File.Source = dsFileHandle->GetDStorageFile();
						dsRequest.Source.File.Offset = tileDataInfo.offset;
						dsRequest.Source.File.Size = tileDataInfo.numBytes;
						dsRequest.UncompressedSize = reTextureFileHeader.tileSlicePitch;

						dstorageQueue->EnqueueRequest(&dsRequest);

						currTileIndex++;
					}

					currSubresourceIndex++;
				}
			}

			dstorageFence->IncrementExpectedValue();
			dstorageQueue->EnqueueSignal(*dstorageFence);
			dstorageQueue->Submit();
		};
		preloadTerrainTextureArray(mRenderer->mNearTerrainAlbedoArray.get(), mBackMappingQueue.get(), mBackMappingFence.get(), mBackDStorageQueue.get(), mBackDStorageFence.get());
		preloadTerrainTextureArray(mRenderer->mNearTerrainNormalArray.get(), mBackMappingQueue.get(), mBackMappingFence.get(), mBackDStorageQueue.get(), mBackDStorageFence.get());

		// Ԥ����TerrainTiledSplatMap
		auto preloadTerrainTiledTexture = [&](TerrainTiledTexture* terrainTiledTexture, GHL::CommandQueue* mappingQueue, GHL::Fence* mappingFence, GHL::DirectStorageQueue* dstorageQueue, GHL::Fence* dstorageFence, Math::Vector4 cameraPosition) {
			const auto& reTextureFileFormat = terrainTiledTexture->GetReTextureFileFormat();
			const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
			const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();

			auto* dsFileHandle = terrainTiledTexture->GetDStorageFile();
			auto& tiledTexture = terrainTiledTexture->GetTiledTexture();

			std::vector<TerrainTiledTextureTileRequestTask> requestTasks;
			ProduceTerrainTiledSplatMapTileRequest(requestTasks, cameraPosition, false);

			for (const auto& requestTask : requestTasks) {
				const auto& tileDataInfo = reTileDataInfos.at(requestTask.nextTileIndex);
				uint32_t tileCountPerAxis = reTextureFileHeader.imageWidth / reTextureFileHeader.tileWidth;
				uint32_t tilePosX = requestTask.nextTileIndex % tileCountPerAxis;
				uint32_t tilePosY = requestTask.nextTileIndex / tileCountPerAxis;
				uint32_t subresourceIndex = 0u;
				uint32_t numTiles = 1u;
				auto* heapAllocation = requestTask.cacheNode->heapAllocation;

				std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags(numTiles, D3D12_TILE_RANGE_FLAG_NONE);
				// if the number of standard (not packed) mips is n, then start updating at subresource n
				D3D12_TILED_RESOURCE_COORDINATE tiledRegionStartCoordinates{ tilePosX, tilePosY, 0, subresourceIndex };
				D3D12_TILE_REGION_SIZE tiledRegionSizes{ numTiles, FALSE, 0, 0, 0 };

				const UINT w = mRenderer->mTerrainTiledSplatMapBackendTiling.WidthInTiles;
				UINT y = heapAllocation->tileOffset / w;
				UINT x = heapAllocation->tileOffset - (w * y);
				D3D12_TILED_RESOURCE_COORDINATE backendTiledRegionStartCoordinates{ x, y, 0, subresourceIndex };

				DSTORAGE_REQUEST dsRequest{};
				dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
				dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TILES;
				dsRequest.Options.CompressionFormat = (DSTORAGE_COMPRESSION_FORMAT)reTextureFileHeader.compressionFormat;
				dsRequest.Destination.Tiles.Resource = mRenderer->mTerrainTiledSplatMapBackend.Get();
				dsRequest.Destination.Tiles.TiledRegionStartCoordinate = backendTiledRegionStartCoordinates;
				dsRequest.Destination.Tiles.TileRegionSize = tiledRegionSizes;
				dsRequest.Source.File.Source = dsFileHandle->GetDStorageFile();
				dsRequest.Source.File.Offset = tileDataInfo.offset;
				dsRequest.Source.File.Size = tileDataInfo.numBytes;
				dsRequest.UncompressedSize = reTextureFileHeader.tileSlicePitch;

				dstorageQueue->EnqueueRequest(&dsRequest);
				dstorageFence->IncrementExpectedValue();
				dstorageQueue->EnqueueSignal(*dstorageFence);
				dstorageQueue->Submit();
				dstorageFence->Wait();

				// perform packed mip tile mapping on the copy queue
				mappingQueue->D3DCommandQueue()->UpdateTileMappings(
					tiledTexture->D3DResource(),
					numTiles,
					&tiledRegionStartCoordinates,
					&tiledRegionSizes,
					heapAllocation->heap->D3DHeap(),
					numTiles,
					rangeFlags.data(),
					&heapAllocation->tileOffset,
					nullptr,
					D3D12_TILE_MAPPING_FLAG_NONE
				);
				mappingFence->IncrementExpectedValue();
				mappingQueue->SignalFence(*mappingFence);
				mappingFence->Wait();
			}

			// GPU������ɺ󣬸���CPU�нڵ���Դפ��״̬
			auto* heapAllocationCache = mRenderer->mTerrainTiledSplatMapHeapAllocationCache.get();
			for (const auto& requestTask : requestTasks) {
				auto& currTileRuntimeState = mTerrainTiledTextureTileRuntimeStates.at(requestTask.nextTileIndex);
				currTileRuntimeState.SetInTexture();
				currTileRuntimeState.cacheNode = requestTask.cacheNode;

				requestTask.cacheNode->tileIndex = requestTask.nextTileIndex;
				heapAllocationCache->AddTail(requestTask.cacheNode);
			}
		};
		preloadTerrainTiledTexture(mRenderer->mTerrainTiledSplatMap.get(), mBackMappingQueue.get(), mBackMappingFence.get(), mBackDStorageQueue.get(), mBackDStorageFence.get(), resourceStorage->rootConstantsPerFrame.currentEditorCamera.position);

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
				RecordFarTerrainTextureAtlas(mRenderer->mFarTerrainHeightMapAtlas.get(), requestTask, copyCommandList, mBackCopyLinearBufferAllocator.get());
				RecordFarTerrainTextureAtlas(mRenderer->mFarTerrainAlbedoMapAtlas.get(), requestTask, copyCommandList, mBackCopyLinearBufferAllocator.get());
				RecordFarTerrainTextureAtlas(mRenderer->mFarTerrainNormalMapAtlas.get(), requestTask, copyCommandList, mBackCopyLinearBufferAllocator.get());

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
			mBackCopyQueue->SignalFence(*mBackCopyFence.get());
		}

		// ѹ���µ�ComputeFrame
		{
			mBackComputeFence->IncrementExpectedValue();
			mBackComputeFrameTracker->PushCurrentFrame(mBackComputeFence->ExpectedValue());
		}

		{
			// mBackComputeQueue->WaitFence(*mBackDStorageFence.get());
			// mBackComputeQueue->WaitFence(*mBackCopyFence.get());

			auto commandList = mBackComputeCommandListAllocator->AllocateComputeCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mBackResourceStateTracker.get(), mBackComputeLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateTerrainNodeDescriptorPass");

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

		// ����֡��ɻص�����
		SetupFrameCompletedCallBack();

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

				// ���ݵ�ǰ�����������λ�ü�����Ҫ���µĵ��νڵ�
				auto cameraPosition = pipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.position;
				std::vector<TerrainNodeRequestTask> terrainNodeRequestTasks;
				ProduceTerrainNodeRequest(terrainNodeRequestTasks, cameraPosition);

				// ��ǰ�нڵ��������
				if (!terrainNodeRequestTasks.empty()) {	
					// ѹ���µ�CopyFrame��ComputeFrame
					uint64_t currFrameCopyFenceExpectedValue = mBackCopyFence->IncrementExpectedValue();
					uint64_t currFrameComputeFenceExpectedValue = mBackComputeFence->IncrementExpectedValue();

					mBackCopyFrameTracker->PushCurrentFrame(currFrameCopyFenceExpectedValue);
					mBackComputeFrameTracker->PushCurrentFrame(currFrameComputeFenceExpectedValue);

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
					recordedGpuCommand.frameIndex = mBackCopyFrameTracker->GetCurrFrameIndex();

					// ���ݵ�ǰ��������¼��GPU����
					RecordGpuCommand(terrainNodeRequestTasks, recordedGpuCommand);

					// Ԥ�����������Ա����֡��ɺ�Ļص�����(mBackComputeFrameTracker->GetFrameIndex()������Ҳ����)
					mReservedTerrainNodeRequestTasks.at(mBackCopyFrameTracker->GetCurrFrameIndex()).insert(mReservedTerrainNodeRequestTasks.at(mBackCopyFrameTracker->GetCurrFrameIndex()).end(), terrainNodeRequestTasks.begin(), terrainNodeRequestTasks.end());
					mFrameCompletedFlags.at(mBackCopyFrameTracker->GetCurrFrameIndex()) = 0u;				// ˢ��֡��ɱ��

					// ������ѹ�������
					mRecordedGpuCommands.Push(std::move(recordedGpuCommand));
				}

				std::vector<TerrainTiledTextureTileRequestTask> terrainTiledTextureTileRequestTasks;
				ProduceTerrainTiledSplatMapTileRequest(terrainTiledTextureTileRequestTasks, cameraPosition);

				if (!terrainTiledTextureTileRequestTasks.empty()) {
					// ѹ���µ�DStorageFrame
					uint64_t currFrameMappingFenceExpectedValue = mBackMappingFence->IncrementExpectedValue();
					mBackDStorageFrameTracker->PushCurrentFrame(currFrameMappingFenceExpectedValue);

					ProcessTerrainTiledSplatMapTileRequest(terrainTiledTextureTileRequestTasks);

					// Ԥ����������
					mReservedTerrainTiledTextureTileRequestTasks.at(mBackDStorageFrameTracker->GetCurrFrameIndex()).insert(mReservedTerrainTiledTextureTileRequestTasks.at(mBackDStorageFrameTracker->GetCurrFrameIndex()).end(), terrainTiledTextureTileRequestTasks.begin(), terrainTiledTextureTileRequestTasks.end());

					mBackMappingQueue->SignalFence(*mBackMappingFence.get());
				}
			}

			// ��������GPU�����Ƿ����
			if (mBackCopyFrameTracker->GetUsedSize() == smMaxBackFrameCount) {
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

				UINT64 valueToWaitFor = mBackCopyFence->ExpectedValue() - (smMaxBackFrameCount - 1u);
				mBackCopyFence->SetCompletionEvent(valueToWaitFor, eventHandle);

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}
			if (mBackComputeFrameTracker->GetUsedSize() == smMaxBackFrameCount) {
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

				UINT64 valueToWaitFor = mBackComputeFence->ExpectedValue() - (smMaxBackFrameCount - 1u);
				mBackComputeFence->SetCompletionEvent(valueToWaitFor, eventHandle);

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}
			if (mBackDStorageFrameTracker->GetUsedSize() == smMaxBackFrameCount) {
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

				UINT64 valueToWaitFor = mBackDStorageFence->ExpectedValue() - (smMaxBackFrameCount - 1u);
				mBackDStorageFence->SetCompletionEvent(valueToWaitFor, eventHandle);

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}

			// ��Ⲣ������Ⱦ֡�Ƿ����
			mBackCopyFrameTracker->PopCompletedFrame(mBackCopyFence->CompletedValue());
			mBackComputeFrameTracker->PopCompletedFrame(mBackComputeFence->CompletedValue());
			mBackDStorageFrameTracker->PopCompletedFrame(mBackMappingFence->CompletedValue());

			if (!mThreadRunning) {
				break;
			}
		}
	}

	void TerrainBackend::ProduceTerrainNodeRequest(std::vector<TerrainNodeRequestTask>& requestTasks, Math::Vector3 cameraPosition) {
		auto* tileCache = mRenderer->mFarTerrainTextureAtlasTileCache.get();

		auto GetFixedPos = [](const Math::Vector3& pos, float nodeMeterSize, float terrainMeterSize) {
			// ��pos��ƫ��
			Math::Vector3 transformedPos = pos;
			transformedPos.x = transformedPos.x + terrainMeterSize / 2.0f;
			transformedPos.y = transformedPos.y;
			transformedPos.z = -transformedPos.z + terrainMeterSize / 2.0f;

			Math::Int2 fixedPos = Math::Int2{
				(int32_t)std::floor(transformedPos.x / nodeMeterSize + 0.5f) * (int32_t)nodeMeterSize,
				(int32_t)std::floor(transformedPos.z / nodeMeterSize + 0.5f) * (int32_t)nodeMeterSize
			};

			return Math::Int2{ fixedPos.x / (int32_t)nodeMeterSize, fixedPos.y / (int32_t)nodeMeterSize };
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
			Math::Int2 fixedPos = GetFixedPos(cameraPosition, (float)currLodDescriptor.nodeMeterSize, mTerrainSetting.smTerrainMeterSize);
			// ���Ƿ�ΧΪ�� -smsmTerrainDataRange, smTerrainDataRange �ڵĵ��νڵ�
			for (int32_t yIndex = -mTerrainSetting.smTerrainDataLoadedRange; yIndex <= mTerrainSetting.smTerrainDataLoadedRange; yIndex++) {
				for (int32_t xIndex = -mTerrainSetting.smTerrainDataLoadedRange; xIndex <= mTerrainSetting.smTerrainDataLoadedRange; xIndex++) {
					int32_t currNodeLocationX = fixedPos.x + xIndex;
					int32_t currNodeLocationY = fixedPos.y + yIndex;

					// �޳��Ƿ��߽�
					if (currNodeLocationX < 0 || currNodeLocationX > ((mTerrainSetting.smTerrainMeterSize / currLodDescriptor.nodeMeterSize) - 1)) {
						continue;
					}
					if (currNodeLocationY < 0 || currNodeLocationY > ((mTerrainSetting.smTerrainMeterSize / currLodDescriptor.nodeMeterSize) - 1)) {
						continue;
					}

					// �������ǰ�ڵ��ȫ������
					uint32_t nodeCountPerRow = mTerrainSetting.smTerrainMeterSize/ currLodDescriptor.nodeMeterSize;
					uint32_t currNodeIndex = currLodDescriptor.nodeStartOffset + currNodeLocationY * nodeCountPerRow + currNodeLocationX;

					// ��ȡ�ýڵ��ʵʱ״̬
					auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(currNodeIndex);

					if (currNodeRuntimeState.inReady || currNodeRuntimeState.inQueue || currNodeRuntimeState.inLoading) {
						// �ýڵ��Ӧ����Դ���ڼ���
						continue;
					}
					// �ýڵ��Ӧ����Դ����ͼ����
					else if (!currNodeRuntimeState.inReadyOut && !currNodeRuntimeState.inQueueOut && !currNodeRuntimeState.inLoadingOut && currNodeRuntimeState.inTexture) {
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

		// Ϊʣ��������������atlasNode�������½ڵ��ʵʱ״̬
		for (auto& requestTask : requestTasks) {
			auto* atlasNode = tileCache->GetHead();
			tileCache->Remove(atlasNode);

			// �����atlasNode�Ѿ�������һ�����νڵ����Դ������¼
			if (atlasNode->terrainNodeIndex != -1) {
				requestTask.prevTerrainNodeIndex = atlasNode->terrainNodeIndex;
			}
			// ʹ�õ�atlasNodeû�и��ص��νڵ�
			else {
				requestTask.prevTerrainNodeIndex = 65536u;
			}
			requestTask.atlasNode = atlasNode;

			auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.nextTerrainNodeIndex);
			currNodeRuntimeState.SetInReady();
			if (requestTask.prevTerrainNodeIndex != 65536u) {
				auto& prevNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.prevTerrainNodeIndex);
				prevNodeRuntimeState.SetInReadyOut();
			}
		}
	}

	void TerrainBackend::ProduceTerrainTiledSplatMapTileRequest(std::vector<TerrainTiledTextureTileRequestTask>& requestTasks, Math::Vector3 cameraPosition, bool useLimit) {
		auto* terrainTiledSplatMap = mRenderer->mTerrainTiledSplatMap.get();
		auto* heapAllocatonCache = mRenderer->mTerrainTiledSplatMapHeapAllocationCache.get();

		const auto& reTextureFileFormat = terrainTiledSplatMap->GetReTextureFileFormat();
		const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
		const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();

		auto GetFixedPos = [](const Math::Vector3& pos, float tileMeterSize, float terrainMeterSize) {
			// ��pos��ƫ��
			Math::Vector3 transformedPos = pos;
			transformedPos.x = transformedPos.x + terrainMeterSize / 2.0f;
			transformedPos.y = transformedPos.y;
			transformedPos.z = -transformedPos.z + terrainMeterSize / 2.0f;

			Math::Int2 fixedPos = Math::Int2{
				(int32_t)std::floor(transformedPos.x / tileMeterSize + 0.5f) * (int32_t)tileMeterSize,
				(int32_t)std::floor(transformedPos.z / tileMeterSize + 0.5f) * (int32_t)tileMeterSize
			};

			return Math::Int2{ fixedPos.x / (int32_t)tileMeterSize, fixedPos.y / (int32_t)tileMeterSize };
		};

		Math::Int2 fixedPos = GetFixedPos(cameraPosition, (float)reTextureFileHeader.tileWidth, mTerrainSetting.smTerrainMeterSize);

		// ���Ƿ�ΧΪ�� -smTerrainTiledTextureDataLoadedRange, smTerrainTiledTextureDataLoadedRange �ڵĵ��νڵ�
		for (int32_t yIndex = -mTerrainSetting.smTerrainTiledTextureDataLoadedRange; yIndex <= mTerrainSetting.smTerrainTiledTextureDataLoadedRange; yIndex++) {
			for (int32_t xIndex = -mTerrainSetting.smTerrainTiledTextureDataLoadedRange; xIndex <= mTerrainSetting.smTerrainTiledTextureDataLoadedRange; xIndex++) {
				int32_t currNodeLocationX = fixedPos.x + xIndex;
				int32_t currNodeLocationY = fixedPos.y + yIndex;

				// �޳��Ƿ��߽�
				if (currNodeLocationX < 0 || currNodeLocationX > ((reTextureFileHeader.imageWidth / reTextureFileHeader.tileWidth) - 1)) {
					continue;
				}
				if (currNodeLocationY < 0 || currNodeLocationY > ((reTextureFileHeader.imageWidth / reTextureFileHeader.tileWidth) - 1)) {
					continue;
				}

				// �������ǰ�ڵ��ȫ������
				uint32_t tileCountPerRow = reTextureFileHeader.imageWidth / reTextureFileHeader.tileWidth;
				uint32_t currTileIndex = currNodeLocationY * tileCountPerRow + currNodeLocationX;

				// ��ȡ�ýڵ��ʵʱ״̬
				auto& currTileRuntimeState = mTerrainTiledTextureTileRuntimeStates.at(currTileIndex);

				if (currTileRuntimeState.inReady || currTileRuntimeState.inQueue || currTileRuntimeState.inLoading) {
					// �ýڵ��Ӧ����Դ���ڼ���
					continue;
				}
				// �ýڵ��Ӧ����Դ����ͼ����
				else if (!currTileRuntimeState.inReadyOut && !currTileRuntimeState.inQueueOut && !currTileRuntimeState.inLoadingOut && currTileRuntimeState.inTexture) {
					heapAllocatonCache->Remove(currTileRuntimeState.cacheNode);
					heapAllocatonCache->AddTail(currTileRuntimeState.cacheNode);
				}
				else {
					// ������Ӧ��Tile��������(��������HeapAllocation)
					TerrainTiledTextureTileRequestTask requestTask{};
					requestTask.nextTileIndex = currTileIndex;
					requestTasks.push_back(requestTask);
				}
			}
		}

		// ����һ����������������һ��ֻ����smTerrainDataLoadedLimit�����νڵ������
		if (requestTasks.size() > mTerrainSetting.smTerrainTiledTextureDataLoadedLimit && useLimit) {
			requestTasks.resize(mTerrainSetting.smTerrainTiledTextureDataLoadedLimit);
		}

		// Ϊʣ��������������atlasNode�������½ڵ��ʵʱ״̬
		for (auto& requestTask : requestTasks) {
			auto* cacheNode = heapAllocatonCache->GetHead();
			heapAllocatonCache->Remove(cacheNode);

			// �����atlasNode�Ѿ�������һ�����νڵ����Դ������¼
			if (cacheNode->tileIndex != -1) {
				requestTask.prevTileIndex = cacheNode->tileIndex;
			}
			// ʹ�õ�atlasNodeû�и��ص��νڵ�
			else {
				requestTask.prevTileIndex = -1;
			}
			requestTask.cacheNode = cacheNode;

			auto& currNodeRuntimeState = mTerrainTiledTextureTileRuntimeStates.at(requestTask.nextTileIndex);
			currNodeRuntimeState.SetInReady();
			if (requestTask.prevTileIndex != -1) {
				auto& prevNodeRuntimeState = mTerrainTiledTextureTileRuntimeStates.at(requestTask.prevTileIndex);
				prevNodeRuntimeState.SetInReadyOut();
			}
		}
	}

	void TerrainBackend::ProcessTerrainTiledSplatMapTileRequest(std::vector<TerrainTiledTextureTileRequestTask>& requestTasks) {
		// 1.Mapping HeapAllocation
		// 2.Submit Request
		auto* terrainTiledTexture = mRenderer->mTerrainTiledSplatMap.get();
		auto* tiledTextureBackend = mRenderer->mTerrainTiledSplatMapBackend.Get();
		const auto& tiledTextureBackendTiling = mRenderer->mTerrainTiledSplatMapBackendTiling;
		const auto& reTextureFileFormat = terrainTiledTexture->GetReTextureFileFormat();
		const auto& reTextureFileHeader = reTextureFileFormat.GetFileHeader();
		const auto& reTileDataInfos = reTextureFileFormat.GetTileDataInfos();

		auto* dsFileHandle = terrainTiledTexture->GetDStorageFile();
		auto& tiledTexture = terrainTiledTexture->GetTiledTexture();

		for (const auto& requestTask : requestTasks) {
			const auto& tileDataInfo = reTileDataInfos.at(requestTask.nextTileIndex);
			uint32_t subresourceIndex = 0u;
			uint32_t numTiles = 1u;
			auto* heapAllocation = requestTask.cacheNode->heapAllocation;

			const UINT w = tiledTextureBackendTiling.WidthInTiles;
			UINT y = heapAllocation->tileOffset / w;
			UINT x = heapAllocation->tileOffset - (w * y);
			D3D12_TILED_RESOURCE_COORDINATE tiledRegionStartCoordinates{ x, y, 0, subresourceIndex };
			D3D12_TILE_REGION_SIZE tiledRegionSizes{ numTiles, FALSE, 0, 0, 0 };

			DSTORAGE_REQUEST dsRequest{};
			dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TILES;
			dsRequest.Options.CompressionFormat = (DSTORAGE_COMPRESSION_FORMAT)reTextureFileHeader.compressionFormat;
			dsRequest.Destination.Tiles.Resource = tiledTextureBackend;
			dsRequest.Destination.Tiles.TiledRegionStartCoordinate = tiledRegionStartCoordinates;
			dsRequest.Destination.Tiles.TileRegionSize = tiledRegionSizes;
			dsRequest.Source.File.Source = dsFileHandle->GetDStorageFile();
			dsRequest.Source.File.Offset = tileDataInfo.offset;
			dsRequest.Source.File.Size = tileDataInfo.numBytes;
			dsRequest.UncompressedSize = reTextureFileHeader.tileSlicePitch;

			mBackDStorageQueue->EnqueueRequest(&dsRequest);
		}

		mBackDStorageFence->IncrementExpectedValue();
		mBackDStorageQueue->EnqueueSignal(*mBackDStorageFence.get());
		mBackDStorageQueue->Submit();
		mBackMappingQueue->WaitFence(*mBackDStorageFence.get());

		for (const auto& requestTask : requestTasks) {
			const auto& tileDataInfo = reTileDataInfos.at(requestTask.nextTileIndex);
			uint32_t tileCountPerAxis = reTextureFileHeader.imageWidth / reTextureFileHeader.tileWidth;
			uint32_t tilePosX = requestTask.nextTileIndex % tileCountPerAxis;
			uint32_t tilePosY = requestTask.nextTileIndex / tileCountPerAxis;
			uint32_t subresourceIndex = 0u;
			uint32_t numTiles = 1u;
			auto* heapAllocation = requestTask.cacheNode->heapAllocation;

			std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags(numTiles, D3D12_TILE_RANGE_FLAG_NONE);
			// if the number of standard (not packed) mips is n, then start updating at subresource n
			D3D12_TILED_RESOURCE_COORDINATE tiledRegionStartCoordinates{ tilePosX, tilePosY, 0, subresourceIndex };
			D3D12_TILE_REGION_SIZE tiledRegionSizes{ numTiles, FALSE, 0, 0, 0 };

			// perform packed mip tile mapping on the copy queue
			mBackMappingQueue->D3DCommandQueue()->UpdateTileMappings(
				tiledTexture->D3DResource(),
				numTiles,
				&tiledRegionStartCoordinates,
				&tiledRegionSizes,
				heapAllocation->heap->D3DHeap(),
				numTiles,
				rangeFlags.data(),
				&heapAllocation->tileOffset,
				nullptr,
				D3D12_TILE_MAPPING_FLAG_NONE
			);
		}
	}

	void TerrainBackend::RecordGpuCommand(std::vector<TerrainNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand) {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();

		std::vector<GpuUpdateTerrainNodeDescriptorRequest> updateTerrainNodeDescriptorRequests;
		// ¼��CopyCommandList
		{
			auto copyCommandList = mBackCopyCommandListAllocator->AllocateCopyCommandList();
			::PIXBeginEvent(copyCommandList->D3DCommandList(), 0, Tool::StrUtil::UTF8ToWString("UpdateTerrainAtlasTexturePass").c_str());
			for (auto& requestTask : requestTasks) {
				uint32_t currNodeIndex = requestTask.nextTerrainNodeIndex;

				// ������������
				GpuUpdateTerrainNodeDescriptorRequest updateTerrainNodeDescriptorRequest{};
				updateTerrainNodeDescriptorRequest.srcTerrainNodeIndex = requestTask.prevTerrainNodeIndex;
				updateTerrainNodeDescriptorRequest.dstTerrainNodeIndex = requestTask.nextTerrainNodeIndex;
				updateTerrainNodeDescriptorRequest.tilePosX = requestTask.atlasNode->tilePos.x;
				updateTerrainNodeDescriptorRequest.tilePosY = requestTask.atlasNode->tilePos.y;
				updateTerrainNodeDescriptorRequests.push_back(updateTerrainNodeDescriptorRequest);

				RecordFarTerrainTextureAtlas(mRenderer->mFarTerrainHeightMapAtlas.get(), requestTask, copyCommandList, mBackCopyLinearBufferAllocator.get());
				RecordFarTerrainTextureAtlas(mRenderer->mFarTerrainAlbedoMapAtlas.get(), requestTask, copyCommandList, mBackCopyLinearBufferAllocator.get());
				RecordFarTerrainTextureAtlas(mRenderer->mFarTerrainNormalMapAtlas.get(), requestTask, copyCommandList, mBackCopyLinearBufferAllocator.get());

				auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.nextTerrainNodeIndex);
				currNodeRuntimeState.SetInQueue();
				if (requestTask.prevTerrainNodeIndex != 65536u) {
					auto& prevNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.prevTerrainNodeIndex);
					prevNodeRuntimeState.SetInQueueOut();
				}
			}
			::PIXEndEvent(copyCommandList->D3DCommandList());
			copyCommandList->Close();
			recordedGpuCommand.copyCommandList = copyCommandList.Get();
		}

		// ¼��ComputeCommandList
		{
			auto computeCommandList = mBackComputeCommandListAllocator->AllocateComputeCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			computeCommandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ computeCommandList.Get(), shaderManger, mBackResourceStateTracker.get(), mBackComputeLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateTerrainNodeDescriptorPass");

			// Pass Data
			mUpdateTerrainNodeDescriptorPassData.terrainNodeDescriptorBufferIndex = mRenderer->mTerrainNodeDescriptorBuffer->GetUADescriptor()->GetHeapIndex();
			mUpdateTerrainNodeDescriptorPassData.updateTerrainNodeDescriptorRequestBufferIndex = mUpdateTerrainNodeDescriptorRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			auto passDataAlloc = mBackComputeLinearBufferAllocator->Allocate(sizeof(UpdateTerrainNodeDescriptorPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateTerrainNodeDescriptorPassData, sizeof(UpdateTerrainNodeDescriptorPassData));

			// Upload Data
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch =  mBackResourceStateTracker->TransitionImmediately(mUpdateTerrainNodeDescriptorRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mUpdateTerrainNodeDescriptorRequestBuffer, 0u, updateTerrainNodeDescriptorRequests.data(), updateTerrainNodeDescriptorRequests.size() * sizeof(GpuUpdateTerrainNodeDescriptorRequest));

			barrierBatch =  GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{mRenderer->mTerrainNodeDescriptorBuffer, GHL::EResourceState::NonPixelShaderAccess, GHL::EResourceState::UnorderedAccess} };
			barrierBatch += mBackResourceStateTracker->TransitionImmediately(mUpdateTerrainNodeDescriptorRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetComputeRootSignature();
			commandBuffer.SetComputePipelineState(smUpdateTerrainNodeDescriptorSN);
			commandBuffer.SetComputeRootCBV(1u, passDataAlloc.gpuAddress);

			uint32_t threadGroupCountX = (updateTerrainNodeDescriptorRequests.size() + smThreadSizeInGroup - 1) / smThreadSizeInGroup;
			commandBuffer.Dispatch(threadGroupCountX, 1u, 1u);

			commandBuffer.PIXEndEvent();
			computeCommandList->Close();
			recordedGpuCommand.computeCommandList = computeCommandList.Get();
		}
	}

	void TerrainBackend::RecordFarTerrainTextureAtlas(TerrainTextureAtlas* terrainTextureAtlas, TerrainNodeRequestTask& requestTask, CommandListWrap& copyCommandList, LinearBufferAllocator* tempLinearBufferAllocator) {
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
	}

	void TerrainBackend::OnFrameLoading(uint32_t frameIndex) {
		auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks.at(frameIndex);

		for (auto& requestTask : reservedRequestTasks) {
			// ���½ڵ�ʵʱ״̬
			auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.nextTerrainNodeIndex);
			currNodeRuntimeState.SetInLoading();
			if (requestTask.prevTerrainNodeIndex != 65536u) {
				auto& prevNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.prevTerrainNodeIndex);
				prevNodeRuntimeState.SetInLoadingOut();
			}
		}
	}

	void TerrainBackend::OnFrameCompleted(uint8_t frameIndex) {
		auto* terrainTextureAtlasTileCache = mRenderer->mFarTerrainTextureAtlasTileCache.get();

		auto& currFrameCompletedFlag = mFrameCompletedFlags.at(frameIndex);
		currFrameCompletedFlag++;

		// ����CopyFrame��ComputeFrame�������
		if (currFrameCompletedFlag == smFrameCompletedFlag) {
			
			auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks.at(frameIndex);
			for (auto& requestTask : reservedRequestTasks) {
				// ���½ڵ�ʵʱ״̬
				auto& currNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.nextTerrainNodeIndex);
				currNodeRuntimeState.SetInTexture();
				currNodeRuntimeState.atlasNode = requestTask.atlasNode;
				if (requestTask.prevTerrainNodeIndex != 65536u) {
					auto& prevNodeRuntimeState = mTerrainNodeRuntimeStates.at(requestTask.prevTerrainNodeIndex);
					prevNodeRuntimeState.SetOutTexture();
					prevNodeRuntimeState.atlasNode = nullptr;
				}

				// ����AtlasNode����
				requestTask.atlasNode->terrainNodeIndex = requestTask.nextTerrainNodeIndex;
				terrainTextureAtlasTileCache->AddTail(requestTask.atlasNode);
			}
			reservedRequestTasks.clear();
		}
	}

	void TerrainBackend::OnDStorageFrameCompleted(uint8_t frameIndex) {
		auto* heapAllocationCache = mRenderer->mTerrainTiledSplatMapHeapAllocationCache.get();

		auto& reservedRequestTasks = mReservedTerrainTiledTextureTileRequestTasks.at(frameIndex);
		for (auto& requestTask : reservedRequestTasks) {
			// ���½ڵ�ʵʱ״̬
			auto& currTileRuntimeState = mTerrainTiledTextureTileRuntimeStates.at(requestTask.nextTileIndex);
			currTileRuntimeState.SetInTexture();
			currTileRuntimeState.cacheNode = requestTask.cacheNode;
			if (requestTask.prevTileIndex != -1) {
				auto& prevNodeRuntimeState = mTerrainTiledTextureTileRuntimeStates.at(requestTask.prevTileIndex);
				prevNodeRuntimeState.SetOutTexture();
				prevNodeRuntimeState.cacheNode = nullptr;
			}

			// ����AtlasNode����
			requestTask.cacheNode->tileIndex = requestTask.nextTileIndex;
			heapAllocationCache->AddTail(requestTask.cacheNode);
		}
		reservedRequestTasks.clear();
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
			mBackDStorageQueue = std::make_unique<GHL::DirectStorageQueue>(device, dstorageFactory, DSTORAGE_REQUEST_SOURCE_FILE);
			mBackDStorageFence = std::make_unique<GHL::Fence>(device);
			mBackDStorageFence->SetDebugName("TerrainBackend_DStorageFence");
			mBackDStorageFrameTracker = std::make_unique<Renderer::RingFrameTracker>(smMaxBackFrameCount);
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

		// ����HeapAllocator
		{
			mBackMappingQueue = std::make_unique<GHL::CopyQueue>(device);
			mBackMappingQueue->SetDebugName("TerrainBackend_MappingQueue");
			mBackMappingFence = std::make_unique<GHL::Fence>(device);
			mBackMappingFence->SetDebugName("TerrainBackend_MappingFence");
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
			mUpdateTerrainNodeDescriptorRequestBuffer->SetDebugName("UpdateTerrainNodeDescriptorRequestBuffer");
			mBackResourceStateTracker->StartTracking(mUpdateTerrainNodeDescriptorRequestBuffer);
		}

		// ������ɫ������
		{
			shaderManger->CreateComputeShader(smUpdateTerrainNodeDescriptorSN,
				[&](ComputeStateProxy& proxy) {
					proxy.csFilepath = shaderPath + "TerrainRenderer/TerrainNodeDescriptorUpdater.hlsl";
				});
		}
	}

	void TerrainBackend::SetupFrameCompletedCallBack() {
		// ���FrameCompletedCallback
		mBackCopyFrameTracker->AddFrameCompletedCallBack([this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
			this->OnFrameCompleted(attribute.frameIndex);
			});
		mBackComputeFrameTracker->AddFrameCompletedCallBack([this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
			this->OnFrameCompleted(attribute.frameIndex);
			});

		mBackDStorageFrameTracker->AddFrameCompletedCallBack([this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
			this->OnDStorageFrameCompleted(attribute.frameIndex);
			});
	}

}