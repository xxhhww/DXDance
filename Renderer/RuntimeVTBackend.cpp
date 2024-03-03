#include "Renderer/RuntimeVTBackend.h"
#include "Renderer/RuntimeVTAtlas.h"
#include "Renderer/RuntimeVTAtlasTileCache.h"
#include "Renderer/TerrainTextureArray.h"
#include "Renderer/TerrainTiledTexture.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/Misc.h"

#include "Tools/Assert.h"

namespace Renderer {

	struct GpuUpdateRuntimeVTAtlasRequest {
	public:
		Math::Matrix4 mvpMatrix;
		Math::Vector4 tileOffset;
		Math::Vector4 blendOffset;
	};

	struct GpuUpdateRuntimeVTPageTableRequest {
	public:
		uint32_t tilePosX;
		uint32_t tilePosY;
		int32_t  pageLevel;
		float    pad1;

		Math::Int4 rectInPage0Level;
		Math::Matrix4 mvpMatrix;		// ת����ͼƬ�ռ��еľ���
	};

	RuntimeVTBackend::RuntimeVTBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting)
	: mRenderer(renderer)
	, mTerrainSetting(terrainSetting) 
	, mRvtPageTables(mRenderer->mRuntimeVTPageTables) {
		mReservedTerrainNodeRequestTasks.resize(smMaxRvtFrameCount);

		mHasPreloaded = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mHasPreloaded != nullptr, "CreateEvent Failed");

		CreateGraphicsObject();

		// �����߳�
		mThread = std::thread([this]() {
				this->BackendThread();
			}
		);
	}

	RuntimeVTBackend::~RuntimeVTBackend() {
		mThreadRunning = false;
	}

	void RuntimeVTBackend::Preload() {
		mRuntimeVTRealRect = mRenderer->GetRuntimeVTRealRect();

		// ������ɺ�֪ͨ��̨�̣߳�������Դ�����߳�
		::SetEvent(mHasPreloaded);
	}

	void RuntimeVTBackend::OnFrameLoading(uint32_t frameIndex) {
		auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks.at(frameIndex);

		for (auto& requestTask : reservedRequestTasks) {
			// ���½ڵ�ʵʱ״̬
			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInLoading();
			if (requestTask.prevPageLevel != -1) {
				auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
				prevNodeRuntimeState.SetInLoadingOut();
			}
		}
	}

	void RuntimeVTBackend::BackendThread() {
		// �ȴ�Ԥ�ȼ�����ɺ���������
		::WaitForSingleObject(mHasPreloaded, INFINITE);

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* mainRenderFrameFence = renderEngine->mRenderFrameFence.get();
		uint64_t previousMainFrameFenceValue = 0u;
		bool runtimeVTRealRectChanged = false;

		while (mThreadRunning) {
			uint64_t currentMainFrameFenceValue = mainRenderFrameFence->CompletedValue();

			// �µ�����Ⱦ֡���
			if (previousMainFrameFenceValue != currentMainFrameFenceValue) {
				previousMainFrameFenceValue = currentMainFrameFenceValue;

				std::vector<RuntimeVTNodeRequestTask> rvtNodeRequestTasks;
				runtimeVTRealRectChanged = ProcessTerrainFeedback(rvtNodeRequestTasks, currentMainFrameFenceValue);

				if (!rvtNodeRequestTasks.empty() && !runtimeVTRealRectChanged) {
					// ѹ���µ�RvtFrame
					uint64_t currFrameFenceExpectedValue = mRvtGraphicsFence->IncrementExpectedValue();
					mRvtFrameTracker->PushCurrentFrame(currFrameFenceExpectedValue);

					// ��¼��ǰ֡��Ӧ��Gpu����
					RecordedGpuCommand recordedGpuCommand{};
					recordedGpuCommand.graphicsFence = mRvtGraphicsFence.get();
					recordedGpuCommand.graphicsQueue = mRvtGraphicsQueue.get();
					recordedGpuCommand.graphicsFenceExpectedValue = currFrameFenceExpectedValue;
					recordedGpuCommand.frameIndex = mRvtFrameTracker->GetCurrFrameIndex();
					recordedGpuCommand.updateRuntimeVTPageTableCommandList = nullptr;
					recordedGpuCommand.updateRuntimeVTTextureAtlasCommandList = nullptr;

					// ���ݵ�ǰ��������¼��GPU����
					RecordGpuCommand(rvtNodeRequestTasks, recordedGpuCommand);
					// Ԥ�����������Ա����֡��ɺ�Ļص�����(mBackComputeFrameTracker->GetFrameIndex()������Ҳ����)
					mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).insert(mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).end(), rvtNodeRequestTasks.begin(), rvtNodeRequestTasks.end());

					// ������ѹ�������
					mRecordedGpuCommands.Push(std::move(recordedGpuCommand));
				}
			}

			// ���̼߳�⵽RuntimeVTRealRectChanged�¼����������¼�֪ͨ��RuntimeVTBackend�̣߳���ʱ�����̴߳�ʱͬ���ȴ�RuntimeVTBackend�̵߳Ĵ���
			if (mRenderer->CheckRuntimeVTRealRectChanged() || runtimeVTRealRectChanged) {
				runtimeVTRealRectChanged = false;

				mRuntimeVTRealRect = mRenderer->GetRuntimeVTRealRect();

				// ����RuntimeVTRealRectChanged
				ProcessRuntimeVTRealRectChanged();

				// ֪ͨ���߳�
				mRenderer->SetRuntimeVTRealRectChangedCompletedEvnet();
			}

			// ��������GPU�����Ƿ����
			if (mRvtFrameTracker->GetUsedSize() == smMaxRvtFrameCount) {
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

				UINT64 valueToWaitFor = mRvtGraphicsFence->ExpectedValue() - (smMaxRvtFrameCount - 1u);
				mRvtGraphicsFence->SetCompletionEvent(valueToWaitFor, eventHandle);

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}

			// ��Ⲣ������Ⱦ֡�Ƿ����
			mRvtFrameTracker->PopCompletedFrame(mRvtGraphicsFence->CompletedValue());

			if (!mThreadRunning) {
				break;
			}
		}
	}

	bool RuntimeVTBackend::ProcessTerrainFeedback(std::vector<RuntimeVTNodeRequestTask>& requestTasks, uint32_t completedFenceValue) {
		auto& queuedFeedbackReadbacks = mRenderer->mQueuedFeedbackReadbacks;
		auto& terrainFeedbackReadbackBuffers = mRenderer->mTerrainFeedbackReadbackBuffers;
		
		// Ѱ�������ʵ���RuntimeVTRealRectһ�µ�Feedback
		bool feedbackFound = false;
		uint64_t latestFeedbackFenceValue = 0;
		uint32_t targetFeedbackIndex{ 0u };
		for (size_t i = 0; i < queuedFeedbackReadbacks.size(); i++) {
			if (queuedFeedbackReadbacks.at(i).isFresh && queuedFeedbackReadbacks.at(i).runtimeVTRealRect == mRuntimeVTRealRect) {
				uint64_t feedbackFenceValue = queuedFeedbackReadbacks.at(i).renderFrameFenceValue;
				if ((completedFenceValue >= feedbackFenceValue) && ((!feedbackFound) || (latestFeedbackFenceValue <= feedbackFenceValue))) {
					feedbackFound = true;
					targetFeedbackIndex = i;
					latestFeedbackFenceValue = feedbackFenceValue;
					// this feedback will either be used or skipped. either way it is "consumed"
					queuedFeedbackReadbacks.at(i).isFresh = false;
				}
			}
		}

		if (!feedbackFound) {
			return false;
		}

		mRuntimeVTFrameTick++;
;
		auto& readbackBuffer = terrainFeedbackReadbackBuffers.at(targetFeedbackIndex);
		uint16_t* pResolvedData = reinterpret_cast<uint16_t*>(readbackBuffer->Map());

		auto& feedbackMapDesc = mRenderer->mTerrainFeedbackMap->GetResourceFormat().GetTextureDesc();
		uint32_t width = feedbackMapDesc.width;
		uint32_t height = feedbackMapDesc.height;

		// rowByteSize�ĵ�λ��Byte����������2Byte��ȡ�ģ����Ҫ����2
		uint32_t rowByteSize = (width * GHL::GetFormatStride(feedbackMapDesc.format) + 0x0ff) & ~0x0ff;
		rowByteSize /= (sizeof(uint16_t) / sizeof(uint8_t));

		auto* tileCache = mRenderer->mRuntimeVTAtlasTileCache.get();

		for (uint32_t y = 0u; y < height; y++) {
			for (uint32_t x = 0u; x < rowByteSize;) {
				// ÿһ�ζ��ж��Ƿ���RuntimeVTRealRectChanged�¼�
				if (mRenderer->CheckRuntimeVTRealRectChanged()) {

					// �ָ�PageTableNodeʵʱ״̬
					for (auto& requestTask : requestTasks) {
						auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
						currNodeRuntimeState.SetTempFlag(false);
					}

					return true;
				}

				uint16_t page0PosX = pResolvedData[x++];	// page0PosX
				uint16_t page0PosY = pResolvedData[x++];	// page0PosY
				uint16_t pageLevel = pResolvedData[x++];	// pageLevel
				uint16_t overBound = pResolvedData[x++];	// overBound

				if (overBound == 0u || pageLevel <= 5) { continue; }

				// ��Page0Posת������ǰLevel�µ�PagePos
				int32_t perSize = (int32_t)std::pow(2, pageLevel);
				int32_t pagePosX = page0PosX / perSize;
				int32_t pagePosY = page0PosY / perSize;

				// ����������(ע���޳��ظ�Ŀ��)
				auto& currNodeRuntimeState = mRvtPageTables[pageLevel].GetNodeRuntimeStateTransformed((uint32_t)pagePosX, (uint32_t)pagePosY);

				if (currNodeRuntimeState.tempFlag) {
					// �ýڵ��Ӧ����Դ���ڼ���
					continue;
				}

				// ������Ӧ�ĵ�����������(��������ͼ��Ԫ��)
				RuntimeVTNodeRequestTask requestTask{};
				requestTask.nextPageLevel = (int32_t)pageLevel;
				requestTask.nextPagePos.x = pagePosX;
				requestTask.nextPagePos.y = pagePosY;
				requestTask.runtimeVTRealRect = mRuntimeVTRealRect;
				requestTasks.push_back(requestTask);

				// �޳��ظ�requestTask
				currNodeRuntimeState.SetTempFlag(true);

				/*
				if (currNodeRuntimeState.inReady || currNodeRuntimeState.inQueue || currNodeRuntimeState.inLoading || currNodeRuntimeState.tempFlag) {
					// �ýڵ��Ӧ����Դ���ڼ���
					continue;
				}
				// �ýڵ��Ӧ����Դ����ͼ����
				else if (!currNodeRuntimeState.inReadyOut && !currNodeRuntimeState.inQueueOut && !currNodeRuntimeState.inLoadingOut && currNodeRuntimeState.inTexture) {
					if (currNodeRuntimeState.atlasNode == nullptr) {
						int32_t i = 0;
					}
					currNodeRuntimeState.atlasNode->runtimeVTFrameTick = mRuntimeVTFrameTick;
					tileCache->Remove(currNodeRuntimeState.atlasNode);
					tileCache->AddTail(currNodeRuntimeState.atlasNode);
				}
				else {
					// ������Ӧ�ĵ�����������(��������ͼ��Ԫ��)
					RuntimeVTNodeRequestTask requestTask{};
					requestTask.nextPageLevel = (int32_t)pageLevel;
					requestTask.nextPagePos.x = pagePosX;
					requestTask.nextPagePos.y = pagePosY;
					requestTask.runtimeVTRealRect = mRuntimeVTRealRect;
					requestTasks.push_back(requestTask);

					// �޳��ظ�requestTask
					currNodeRuntimeState.SetTempFlag(true);
				}
				*/
			}

			// ת�Ƶ��������һ�У������ÿһ�г������һ�ж���Ҫ256�ֽڶ���
			pResolvedData += rowByteSize;
		}
		readbackBuffer->UnMap();

		// ������߼����PageLevelһֱ����פ��״̬
		auto& runtimeVTPageTable = mRvtPageTables.back();
		auto& maxPageLevelNodeRuntimeStates = runtimeVTPageTable.GetNodeRuntimeStates();
		for (auto& rows : maxPageLevelNodeRuntimeStates) {
			for (auto& currNodeRuntimeState : rows) {
				if (currNodeRuntimeState.atlasNode != nullptr) {
					tileCache->Remove(currNodeRuntimeState.atlasNode);
					tileCache->AddTail(currNodeRuntimeState.atlasNode);
				}
			}
		}

		// �ָ�PageTableNodeʵʱ״̬
		for (auto& requestTask : requestTasks) {
			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetTempFlag(false);
		}

		// ����pageLevel�Ӵ�С��������н�������
		std::sort(requestTasks.begin(), requestTasks.end(),
			[](const RuntimeVTNodeRequestTask& taskA, const RuntimeVTNodeRequestTask& taskB) {
				return taskA.nextPageLevel > taskB.nextPageLevel;
			}
		);

		// ������������������
		if (requestTasks.size() > mTerrainSetting.smRvtDataLoadedLimit) {
			requestTasks.resize(mTerrainSetting.smRvtDataLoadedLimit);
		}

		// ����Tile
		for (auto& requestTask : requestTasks) {
			auto* atlasNode = tileCache->GetHead();
			tileCache->Remove(atlasNode);

			// �����atlasNode�Ѿ�������һ�����νڵ����Դ������¼
			if (atlasNode->pageLevel != -1) {
				requestTask.prevPagePos = atlasNode->pagePos;
				requestTask.prevPageLevel = atlasNode->pageLevel;
			}
			// ʹ�õ�atlasNodeû�и��ص��νڵ�
			else {
				requestTask.prevPagePos = Math::Int2{ -1, -1 };
				requestTask.prevPageLevel = -1;
			}

			requestTask.atlasNode = atlasNode;

			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInReady();
			if (requestTask.prevPageLevel != -1) {
				auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
				bool inTExture = prevNodeRuntimeState.inTexture;
				prevNodeRuntimeState.SetInReadyOut();
				if (prevNodeRuntimeState.atlasNode == nullptr) {
					int32_t i = 0;
				}
			}
		}

		return false;
	}

	void RuntimeVTBackend::RecordGpuCommand(std::vector<RuntimeVTNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand) {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* mainResourceStateTracker = renderEngine->mResourceStateTracker.get();

		std::vector<GpuUpdateRuntimeVTAtlasRequest> updateRequests;
		for (auto& requestTask : requestTasks) {
			// ����tilePos��Ӧ��ͼ��ռ��µ�tileRect
			const auto& tileSizeWithPadding = mTerrainSetting.smRvtTileSizeWithPadding;
			const Math::Vector4 tileRectInImageSpace = Math::Vector4{
				(float)requestTask.atlasNode->tilePos.x * (float)tileSizeWithPadding,
				(float)requestTask.atlasNode->tilePos.y * (float)tileSizeWithPadding,
				(float)tileSizeWithPadding,
				(float)tileSizeWithPadding
			};

			float powNumber = std::pow(2, requestTask.nextPageLevel);
			const auto& currRvtRealRect = mRenderer->GetRuntimeVTRealRect();
			float paddingEffect = powNumber * mTerrainSetting.smWorldMeterSizePerPaddingInPage0Level;
			Math::Vector4 tileRectInWorldSpace = Math::Vector4{
				currRvtRealRect.x + (mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber * (float)requestTask.nextPagePos.x) - paddingEffect,
				currRvtRealRect.y - (mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber * (float)requestTask.nextPagePos.y) + paddingEffect,
				mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber + 2.0f * paddingEffect,
				mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber + 2.0f * paddingEffect
			};

			// �ؿ����
			const Math::Vector2 terrainMeterSize = mTerrainSetting.smTerrainMeterSize;
			Math::Vector4 terrainRectInWorldSpace = Math::Vector4{
				-mTerrainSetting.smTerrainMeterSize / 2.0f,
				mTerrainSetting.smTerrainMeterSize / 2.0f,
				terrainMeterSize,
				terrainMeterSize
			};

			// ��tileRectInWorldSpace��terrainRectInWorldSpace���ص�
			float tileRectInWorldSpaceOverlappedXMin = std::max(tileRectInWorldSpace.x, terrainRectInWorldSpace.x);
			float tileRectInWorldSpaceOverlappedXMax = std::min(tileRectInWorldSpace.x + tileRectInWorldSpace.z, terrainRectInWorldSpace.x + terrainRectInWorldSpace.z);
			float tileRectInWorldSpaceOverlappedZMin = std::min(tileRectInWorldSpace.y, terrainRectInWorldSpace.y);
			float tileRectInWorldSpaceOverlappedZMax = std::max(tileRectInWorldSpace.y - tileRectInWorldSpace.w, terrainRectInWorldSpace.y - terrainRectInWorldSpace.w);
			Math::Vector4 tileRectInWorldSpaceOverlapped = Math::Vector4{
				tileRectInWorldSpaceOverlappedXMin,
				tileRectInWorldSpaceOverlappedZMin,
				tileRectInWorldSpaceOverlappedXMax - tileRectInWorldSpaceOverlappedXMin,
				tileRectInWorldSpaceOverlappedZMin - tileRectInWorldSpaceOverlappedZMax
			};

			// ����tileRect��WorldSpace��ImageSpace֮������Ź�ϵ
			float scaleFactorWidth = tileRectInImageSpace.z / tileRectInWorldSpace.z;
			float scaleFactorHeight = tileRectInImageSpace.w / tileRectInWorldSpace.w;

			Math::Vector4 tileRectInImageSpaceOverlapped = Math::Vector4{
				tileRectInImageSpace.x + (tileRectInWorldSpaceOverlapped.x - tileRectInWorldSpace.x) * scaleFactorWidth,
				tileRectInImageSpace.y + (tileRectInWorldSpace.y - tileRectInWorldSpaceOverlapped.y) * scaleFactorHeight,
				tileRectInWorldSpaceOverlapped.z * scaleFactorWidth,
				tileRectInWorldSpaceOverlapped.w * scaleFactorHeight
			};

			// tileRect��terrainRect�ĳ���������ԭ�㣨���Ͻǣ�ƽ��
			// �˴������ͼ��
			Math::Vector4 scaleOffset = Math::Vector4{
				tileRectInWorldSpaceOverlapped.z / terrainRectInWorldSpace.z,
				tileRectInWorldSpaceOverlapped.w / terrainRectInWorldSpace.w,
				(tileRectInWorldSpaceOverlapped.x - terrainRectInWorldSpace.x) / terrainRectInWorldSpace.z,
				(terrainRectInWorldSpace.y - tileRectInWorldSpaceOverlapped.y) / terrainRectInWorldSpace.w
			};

			// �˴������ͼ��
			Math::Vector2 nowScale = Math::Vector2(terrainRectInWorldSpace.z / mTerrainSetting.smWorldMeterSizePerTiledTexture, terrainRectInWorldSpace.w / mTerrainSetting.smWorldMeterSizePerTiledTexture);
			Math::Vector4 tileOffset = Math::Vector4(nowScale.x * scaleOffset.x, nowScale.y * scaleOffset.y, nowScale.x * scaleOffset.z, nowScale.y * scaleOffset.w);

			// ����mvpMatrix�����ڽ���Ⱦ������������ͼ����Ӧ��λ����
			float l = tileRectInImageSpaceOverlapped.x * 2.0f / mTerrainSetting.smRvtAtlasTextureSize - 1.0f;
			float r = (tileRectInImageSpaceOverlapped.x + tileRectInImageSpaceOverlapped.z) * 2.0f / mTerrainSetting.smRvtAtlasTextureSize - 1.0f;
			float b = tileRectInImageSpaceOverlapped.y * 2.0f / mTerrainSetting.smRvtAtlasTextureSize - 1.0f;
			float t = (tileRectInImageSpaceOverlapped.y + tileRectInImageSpaceOverlapped.w) * 2.0f / mTerrainSetting.smRvtAtlasTextureSize - 1.0f;
			Math::Matrix4 mvpMatrix{};
			mvpMatrix._11 = 0.0f;
			mvpMatrix._22 = 0.0f;
			mvpMatrix._33 = 0.0f;
			mvpMatrix._44 = 0.0f;
			mvpMatrix._11 = r - l;
			mvpMatrix._14 = l;
			mvpMatrix._22 = t - b;
			mvpMatrix._24 = b;
			mvpMatrix._34 = 1.0f;
			mvpMatrix._44 = 1.0f;

			GpuUpdateRuntimeVTAtlasRequest updateRequest{};
			updateRequest.mvpMatrix = mvpMatrix;
			updateRequest.tileOffset = tileOffset;
			updateRequest.blendOffset = scaleOffset;
			updateRequests.emplace_back(std::move(updateRequest));
		}

		if (updateRequests.empty()) {
			return;
		}

		// ¼��GraphicsCommandList�����ڸ���RuntimeVTAtlas
		{
			auto& terrainAlbedoArray   = mRenderer->GetNearTerrainAlbedoArray()->GetTextureArray();
			auto& terrainNormalArray   = mRenderer->GetNearTerrainNormalArray()->GetTextureArray();
			auto& terrainTiledSplatMap = mRenderer->GetTerrainTiledSplatMap()->GetTiledTexture();
			auto& runtimeVTAlbedoAtlas = mRenderer->GetRuntimeVTAlbedoAtlas()->GetTextureAtlas();
			auto& runtimeVTNormalAtlas = mRenderer->GetRuntimeVTNormalAtlas()->GetTextureAtlas();

			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateRuntimeVTAtlas");

			uint16_t width = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);
			uint16_t height = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);

			mUpdateRuntimeVTAtlasPassData.drawRequestBufferIndex         = mUpdateRuntimeVTAtlasRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainAlbedoTextureArrayIndex = terrainAlbedoArray->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainNormalTextureArrayIndex = terrainNormalArray->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainRoughnessTextureArrayIndex;
			mUpdateRuntimeVTAtlasPassData.terrainSplatMapIndex           = terrainTiledSplatMap->GetSRDescriptor()->GetHeapIndex();

			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRuntimeVTAtlasPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRuntimeVTAtlasPassData, sizeof(UpdateRuntimeVTAtlasPassData));

			// �����������ϴ���GPU��ȥ
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mUpdateRuntimeVTAtlasRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mUpdateRuntimeVTAtlasRequestBuffer, 0u, updateRequests.data(), updateRequests.size() * sizeof(GpuUpdateRuntimeVTAtlasRequest));
		
			barrierBatch = commandBuffer.TransitionImmediately(mUpdateRuntimeVTAtlasRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTAlbedoAtlas, GHL::EResourceState::PixelShaderAccess, GHL::EResourceState::RenderTarget} };
			barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTNormalAtlas, GHL::EResourceState::PixelShaderAccess, GHL::EResourceState::RenderTarget} };

			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetRenderTargets({ runtimeVTAlbedoAtlas, runtimeVTNormalAtlas });
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smUpdateRuntimeVTAtlasSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, updateRequests.size(), 0u, 0u, 0u);

			commandBuffer.PIXEndEvent();
			commandList->Close();
			recordedGpuCommand.updateRuntimeVTTextureAtlasCommandList = commandList.Get();
		}

		// ����GpuUpdateRuntimeVTPageTableRequests;
		std::vector<GpuUpdateRuntimeVTPageTableRequest> updateRuntimeVTPageTableRequests;
		for (const auto& requestTask : requestTasks) {
			int32_t powNumber = std::pow(2, requestTask.nextPageLevel);
			Math::Int2 pagePosInPage0Level = requestTask.nextPagePos * powNumber;

			GpuUpdateRuntimeVTPageTableRequest updateRequest;
			updateRequest.tilePosX = requestTask.atlasNode->tilePos.x;
			updateRequest.tilePosY = requestTask.atlasNode->tilePos.y;
			updateRequest.pageLevel = requestTask.nextPageLevel;
			updateRequest.rectInPage0Level = Math::Int4{ pagePosInPage0Level.x, pagePosInPage0Level.y, powNumber, powNumber };
			updateRuntimeVTPageTableRequests.push_back(updateRequest);

			if (requestTask.prevPageLevel != -1) {
				int32_t pageLevel = requestTask.atlasNode->pageLevel;
				int32_t powNumber = std::pow(2, pageLevel);

				// ��ǰatlasNode�϶�Ӧ��RuntimeVTRealRect�����뵱ǰ��ͬ�������Ҫ��prevPagePosת������ǰRuntimeVTRealRect���ӽ��µ�pagePos
				Math::Int2    pageOffsetInPrevRealRect = requestTask.atlasNode->pageOffset;
				Math::Int2    pagePosInPrevRealRect = requestTask.atlasNode->pagePosInRealRect;

				Math::Vector4 currRealRect = mRuntimeVTRealRect;
				Math::Int2    pageOffsetInCurrRealRect = mRvtPageTables[pageLevel].GetPageOffset();

				Math::Int2    offsetInCurrPageLevel = pageOffsetInCurrRealRect - pageOffsetInPrevRealRect;
				Math::Int2    pagePosInCurrPageLevel = pagePosInPrevRealRect + offsetInCurrPageLevel;
				Math::Int2    pagePosInCurrPage0Level = pagePosInCurrPageLevel * powNumber;
				 
				RuntimeVTPageTableNodeRuntimeState prevNodeRuntimeStates = mRvtPageTables[pageLevel].GetNodeRuntimeStateTransformed(pagePosInCurrPageLevel.x, pagePosInCurrPageLevel.y);

				while (pageLevel <= mRenderer->GetMaxPageLevel() && (prevNodeRuntimeStates.inLoadingOut || prevNodeRuntimeStates.inQueueOut || prevNodeRuntimeStates.inReadyOut || !prevNodeRuntimeStates.inTexture)) {
					pageLevel ++;

					powNumber = std::pow(2, pageLevel);
					pagePosInCurrPageLevel.x = pagePosInCurrPage0Level.x / powNumber;
					pagePosInCurrPageLevel.y = pagePosInCurrPage0Level.y / powNumber;
					prevNodeRuntimeStates = mRvtPageTables[pageLevel].GetNodeRuntimeStateTransformed(pagePosInCurrPageLevel.x, pagePosInCurrPageLevel.y);
				}

				if (prevNodeRuntimeStates.inLoadingOut || prevNodeRuntimeStates.inQueueOut || prevNodeRuntimeStates.inReadyOut) {
					int32_t i = 0;
				}

				updateRequest.tilePosX = prevNodeRuntimeStates.atlasNode->tilePos.x;
				updateRequest.tilePosY = prevNodeRuntimeStates.atlasNode->tilePos.y;
				updateRequest.pageLevel = prevNodeRuntimeStates.pageLevel;
				updateRequest.rectInPage0Level = Math::Int4{ pagePosInCurrPage0Level.x, pagePosInCurrPage0Level.y, powNumber, powNumber };
				updateRuntimeVTPageTableRequests.push_back(updateRequest);
			}
		}

		// ����(�Ӵ�С)
		std::sort(updateRuntimeVTPageTableRequests.begin(), updateRuntimeVTPageTableRequests.end(),
			[](const GpuUpdateRuntimeVTPageTableRequest& requestA, const GpuUpdateRuntimeVTPageTableRequest& requestB) {
				return requestA.pageLevel > requestB.pageLevel;
			});

		// ����MVPMatrix
		for (auto& updateRequest : updateRuntimeVTPageTableRequests) {
			float size = (float)updateRequest.rectInPage0Level.z / (float)mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			updateRequest.mvpMatrix = Math::Matrix4{
				Math::Vector3{ updateRequest.rectInPage0Level.x / (float)mTerrainSetting.smRvtTileCountPerAxisInPage0Level, updateRequest.rectInPage0Level.y / (float)mTerrainSetting.smRvtTileCountPerAxisInPage0Level, 0.0f },
				Math::Quaternion{},
				Math::Vector3{ size, size, size }
			}.Transpose();
		}

		// ¼��GraphicsCommandList�����ڸ���PageTable
		{
			auto& runtimeVTPageTableMap = mRenderer->GetRuntimeVTPageTableMap();

			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateRuntimeVTPageTable");

			uint16_t width = static_cast<uint16_t>(mTerrainSetting.smRvtTileCountPerAxisInPage0Level);
			uint16_t height = static_cast<uint16_t>(mTerrainSetting.smRvtTileCountPerAxisInPage0Level);

			mUpdateRuntimeVTPageTablePassData.drawRequestBufferIndex      = mUpdateRuntimeVTPageTableRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTPageTablePassData.runtimeVTPageTableCopyIndex = mRuntimeVTPageTableCopy->GetUADescriptor()->GetHeapIndex();
			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRuntimeVTPageTablePassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRuntimeVTPageTablePassData, sizeof(UpdateRuntimeVTPageTablePassData));

			{
				static bool flag = true;
				if (flag) {
					auto barrierBatch = GHL::ResourceBarrierBatch{};
					barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTPageTableMap, GHL::EResourceState::PixelShaderAccess, GHL::EResourceState::RenderTarget} };
					commandBuffer.FlushResourceBarrier(barrierBatch);
					commandBuffer.ClearRenderTarget(runtimeVTPageTableMap, Math::Vector4{ 0.0f, 0.0f, (float)mTerrainSetting.smRvtMaxPageLevel + 1u, 0.0f });
					barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTPageTableMap, GHL::EResourceState::RenderTarget, GHL::EResourceState::PixelShaderAccess} };
					commandBuffer.FlushResourceBarrier(barrierBatch);
					flag = false;
				}
			}

			// Copy RuntimeVTPageTable & �����������ϴ���GPU��ȥ
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTPageTableMap, GHL::EResourceState::PixelShaderAccess, GHL::EResourceState::CopySource} };
			barrierBatch += commandBuffer.TransitionImmediately(mRuntimeVTPageTableCopy, GHL::EResourceState::CopyDestination);
			barrierBatch += commandBuffer.TransitionImmediately(mUpdateRuntimeVTPageTableRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.CopyResource(mRuntimeVTPageTableCopy, runtimeVTPageTableMap);
			commandBuffer.UploadBufferRegion(mUpdateRuntimeVTPageTableRequestBuffer, 0u, updateRuntimeVTPageTableRequests.data(), updateRuntimeVTPageTableRequests.size() * sizeof(GpuUpdateRuntimeVTPageTableRequest));

			barrierBatch =  commandBuffer.TransitionImmediately(mRuntimeVTPageTableCopy, GHL::EResourceState::UnorderedAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mUpdateRuntimeVTPageTableRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTPageTableMap, GHL::EResourceState::CopySource, GHL::EResourceState::RenderTarget} };
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.SetRenderTarget(runtimeVTPageTableMap);
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smUpdateRuntimeVTPageTableSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, updateRuntimeVTPageTableRequests.size(), 0u, 0u, 0u);

			commandBuffer.PIXEndEvent();
			commandList->Close();
			recordedGpuCommand.updateRuntimeVTPageTableCommandList = commandList.Get();
		}
	}

	void RuntimeVTBackend::ProcessRuntimeVTRealRectChanged() {
		// ���Ѿ�¼�ƺõ�����ȫ�����
		RecordedGpuCommand recordedGpuCommand{};
		while (!mRecordedGpuCommands.Empty()) {
			mRecordedGpuCommands.TryPop(recordedGpuCommand);

			// CommandList��ִ�У�ִֻ��SignalFence����
			recordedGpuCommand.graphicsQueue->SignalFence(*recordedGpuCommand.graphicsFence, recordedGpuCommand.graphicsFenceExpectedValue);
		}

		// �ָ��ڵ�ʵʱ״̬(֡����ָ�)
		auto* runtimeVTAtlasTileCache = mRenderer->mRuntimeVTAtlasTileCache.get();
		for (int32_t i = mReservedTerrainNodeRequestTasks.size() - 1; i >= 0; i--) {
			auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks[i];
			for (auto& requestTask : reservedRequestTasks) {
				requestTask.atlasNode->pageLevel = -1;
				requestTask.atlasNode->pagePos = Math::Int2{ -1, -1 };

				auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
				currNodeRuntimeState.SetOutTexture();
				currNodeRuntimeState.atlasNode = nullptr;
				if (requestTask.prevPageLevel != -1) {
					auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
					prevNodeRuntimeState.SetOutTexture();
					prevNodeRuntimeState.atlasNode = nullptr;
				}

				ASSERT_FORMAT(requestTask.atlasNode->prev == nullptr);
				ASSERT_FORMAT(requestTask.atlasNode->next == nullptr);
				runtimeVTAtlasTileCache->AddHead(requestTask.atlasNode);
			}
			reservedRequestTasks.clear();
		}

		auto& runtimeVTPageTables = mRenderer->GetRuntimeVTPageTables();
		std::vector<RuntimeVTPageTableNodeRuntimeState*> changedRuntimeVTPageTableNodeRuntimeStates;

		for (auto& runtimeVTPageTable : runtimeVTPageTables) {
			runtimeVTPageTable.OnRuntimeVTRealRectChanged(mRenderer->GetRuntimeVTRealRectOffset(), changedRuntimeVTPageTableNodeRuntimeStates);
		}

		for (auto& changedNodeRuntimeState : changedRuntimeVTPageTableNodeRuntimeStates) {
			// ��changedRuntimeVTPageTableNodeRuntimeStates�п��ܴ����ظ�ֵ����Ҫ�ٽ���nullptr�ж�
			if (changedNodeRuntimeState->atlasNode != nullptr) {
				runtimeVTAtlasTileCache->Remove(changedNodeRuntimeState->atlasNode);
				runtimeVTAtlasTileCache->AddHead(changedNodeRuntimeState->atlasNode);
				changedNodeRuntimeState->atlasNode->pageLevel = -1;
				changedNodeRuntimeState->atlasNode->pagePos = Math::Int2{ -1, -1 };
				changedNodeRuntimeState->atlasNode = nullptr;
				changedNodeRuntimeState->SetOutTexture();
			}
		}
	}

	void RuntimeVTBackend::SetupFrameCompletedCallBack() {
		// ���FrameCompletedCallback
		mRvtFrameTracker->AddFrameCompletedCallBack([this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
			this->OnFrameCompleted(attribute.frameIndex);
		});
	}

	void RuntimeVTBackend::OnFrameCompleted(uint8_t frameIndex) {
		auto* runtimeVTAtlasTileCache = mRenderer->mRuntimeVTAtlasTileCache.get();

		auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks.at(frameIndex);
		for (auto& requestTask : reservedRequestTasks) {
			// ���½ڵ�ʵʱ״̬
			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInTexture();
			currNodeRuntimeState.atlasNode = requestTask.atlasNode;
			if (currNodeRuntimeState.atlasNode == nullptr) {
				int32_t i = 0;
			}
			if (requestTask.prevPageLevel != -1) {
				auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
				prevNodeRuntimeState.SetOutTexture();
				prevNodeRuntimeState.atlasNode = nullptr;
			}

			// ����AtlasNode����
			requestTask.atlasNode->pageLevel = requestTask.nextPageLevel;
			requestTask.atlasNode->pagePos = mRvtPageTables[requestTask.nextPageLevel].GetTransformedXY(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			requestTask.atlasNode->pageOffset = mRvtPageTables[requestTask.nextPageLevel].GetPageOffset();
			requestTask.atlasNode->runtimeVTRealRect = mRuntimeVTRealRect;
			requestTask.atlasNode->pagePosInRealRect = requestTask.nextPagePos;
			runtimeVTAtlasTileCache->AddTail(requestTask.atlasNode);
		}
		reservedRequestTasks.clear();
	}

	void RuntimeVTBackend::CreateGraphicsObject() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageQueue = renderEngine->mDStorageMemQueue.get();
		auto* dstorageFence = renderEngine->mDStorageFence.get();

		auto  shaderPath = renderEngine->smEngineShaderPath;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();

		// ����ͼ��API�������ûص�����
		{
			mRvtResourceStateTracker = std::make_unique<Renderer::ResourceStateTracker>();

			mRvtGraphicsQueue = std::make_unique<GHL::GraphicsQueue>(device);
			mRvtGraphicsQueue->SetDebugName("RuntimeVT_GraphicsQueue");
			mRvtGraphicsFence = std::make_unique<GHL::Fence>(device);
			mRvtGraphicsFence->SetDebugName("RuntimeVT_GraphicsFence");
			mRvtFrameTracker = std::make_unique<Renderer::RingFrameTracker>(smMaxRvtFrameCount);
			mRvtLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mRvtFrameTracker.get());
			mRvtCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mRvtFrameTracker.get());
		}

		// ����RequestBuffer
		{
			auto* runtimeVTAlbedoAtlas = mRenderer->mRuntimeVTAlbedoAtlas.get();

			BufferDesc _UpdateRuntimeVTAtlasRequestBufferDesc{};
			_UpdateRuntimeVTAtlasRequestBufferDesc.stride = sizeof(GpuUpdateRuntimeVTAtlasRequest);
			_UpdateRuntimeVTAtlasRequestBufferDesc.size = _UpdateRuntimeVTAtlasRequestBufferDesc.stride * runtimeVTAlbedoAtlas->GetTileCount();
			_UpdateRuntimeVTAtlasRequestBufferDesc.usage = GHL::EResourceUsage::Default;
			_UpdateRuntimeVTAtlasRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_UpdateRuntimeVTAtlasRequestBufferDesc.initialState = GHL::EResourceState::Common;
			_UpdateRuntimeVTAtlasRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mUpdateRuntimeVTAtlasRequestBuffer = resourceAllocator->Allocate(device, _UpdateRuntimeVTAtlasRequestBufferDesc, descriptorAllocator, nullptr);
			mUpdateRuntimeVTAtlasRequestBuffer->SetDebugName("UpdateRuntimeVTAtlasRequestBuffer");
			mRvtResourceStateTracker->StartTracking(mUpdateRuntimeVTAtlasRequestBuffer);

			BufferDesc _UpdateRuntimeVTPageTableRequestBufferDesc{};
			_UpdateRuntimeVTPageTableRequestBufferDesc.stride = sizeof(GpuUpdateRuntimeVTPageTableRequest);
			_UpdateRuntimeVTPageTableRequestBufferDesc.size = _UpdateRuntimeVTPageTableRequestBufferDesc.stride * runtimeVTAlbedoAtlas->GetTileCount();
			_UpdateRuntimeVTPageTableRequestBufferDesc.usage = GHL::EResourceUsage::Default;
			_UpdateRuntimeVTPageTableRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_UpdateRuntimeVTPageTableRequestBufferDesc.initialState = GHL::EResourceState::Common;
			_UpdateRuntimeVTPageTableRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mUpdateRuntimeVTPageTableRequestBuffer = resourceAllocator->Allocate(device, _UpdateRuntimeVTPageTableRequestBufferDesc, descriptorAllocator, nullptr);
			mUpdateRuntimeVTPageTableRequestBuffer->SetDebugName("UpdateRuntimeVTPageTableRequestBuffer");
			mRvtResourceStateTracker->StartTracking(mUpdateRuntimeVTPageTableRequestBuffer);
		}

		// ����RuntimeVTPageTable����
		{
			TextureDesc _RuntimeVTPageTableMapDesc{};
			_RuntimeVTPageTableMapDesc.width = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			_RuntimeVTPageTableMapDesc.height = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			_RuntimeVTPageTableMapDesc.format = DXGI_FORMAT_R8G8B8A8_UINT;
			_RuntimeVTPageTableMapDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::UnorderedAccess;
			_RuntimeVTPageTableMapDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
			mRuntimeVTPageTableCopy = resourceAllocator->Allocate(device, _RuntimeVTPageTableMapDesc, descriptorAllocator, nullptr);
			mRuntimeVTPageTableCopy->SetDebugName("RuntimeVTPageTableCopy");
			mRvtResourceStateTracker->StartTracking(mRuntimeVTPageTableCopy);
		}

		// ����QuadMesh
		{
			std::vector<Vertex> vertices;
			vertices.resize(6u);
			vertices[0].position = Math::Vector3{ 0.0f, 1.0f, 0.0f };
			vertices[0].uv = Math::Vector2{ 0.0f, 1.0f };
			vertices[1].position = Math::Vector3{ 0.0f, 0.0f, 0.0f };
			vertices[1].uv = Math::Vector2{ 0.0f, 0.0f };
			vertices[2].position = Math::Vector3{ 1.0f, 0.0f, 0.0f };
			vertices[2].uv = Math::Vector2{ 1.0f, 0.0f };
			vertices[3].position = Math::Vector3{ 1.0f, 1.0f, 0.0f };
			vertices[3].uv = Math::Vector2{ 1.0f, 1.0f };

			std::vector<uint32_t> indices;
			indices.emplace_back(0u);
			indices.emplace_back(2u);
			indices.emplace_back(1u);
			indices.emplace_back(2u);
			indices.emplace_back(0u);
			indices.emplace_back(3u);

			Renderer::BufferDesc vertexBufferDesc{};
			vertexBufferDesc.stride = sizeof(Renderer::Vertex);
			vertexBufferDesc.size = vertexBufferDesc.stride * vertices.size();
			vertexBufferDesc.usage = GHL::EResourceUsage::Default;
			mQuadMeshVertexBuffer = resourceAllocator->Allocate(device, vertexBufferDesc, descriptorAllocator, nullptr);

			Renderer::BufferDesc indexBufferDesc{};
			indexBufferDesc.stride = sizeof(uint32_t);
			indexBufferDesc.size = indexBufferDesc.stride * indices.size();
			indexBufferDesc.usage = GHL::EResourceUsage::Default;
			mQuadMeshIndexBuffer = resourceAllocator->Allocate(device, indexBufferDesc, descriptorAllocator, nullptr);
			mQuadMeshIndexCount = indices.size();

			EnqueueDStorageRequest(dstorageQueue, static_cast<void*>(vertices.data()), vertexBufferDesc.size, mQuadMeshVertexBuffer.Get(), 0u);
			EnqueueDStorageRequest(dstorageQueue, static_cast<void*>(indices.data()), indexBufferDesc.size, mQuadMeshIndexBuffer.Get(), 0u);

			dstorageFence->IncrementExpectedValue();
			dstorageQueue->EnqueueSignal(*dstorageFence);
			dstorageQueue->Submit();
			dstorageFence->Wait();
		}

		// ������ɫ������
		{
			shaderManger->CreateGraphicsShader(smUpdateRuntimeVTAtlasSN,
				[&](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = shaderPath + "TerrainRenderer/RuntimeVTAtlasUpdater.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						mRenderer->GetRuntimeVTAlbedoAtlas()->GetDxgiFormat(),		// AlbedoMapAtlas
						mRenderer->GetRuntimeVTNormalAtlas()->GetDxgiFormat()		// NormalMapAtlas
					};
				}
			);

			shaderManger->CreateGraphicsShader(smUpdateRuntimeVTPageTableSN,
				[&](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = shaderPath + "TerrainRenderer/RuntimeVTPageTableUpdater.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						mRenderer->GetRuntimeVTPageTableMap()->GetResourceFormat().GetTextureDesc().format
					};
				}
			);
		}

		// ����֡��ɻص�����
		SetupFrameCompletedCallBack();
	}

}