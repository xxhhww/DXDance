#include "Renderer/RuntimeVirtualTextureBackend.h"
#include "Renderer/RuntimeVirtualTextureAtlasTileCache.h"
#include "Renderer/RenderEngine.h"

#include "Tools/Assert.h"

namespace Renderer {

	RuntimeVirtualTextureBackend::RuntimeVirtualTextureBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting)
	: mRenderer(renderer)
	, mTerrainSetting(terrainSetting) 
	, mRvtPageTables(mRenderer->mRvtPageTables) {

		CreateGraphicsObject();

		// �����߳�
		mThread = std::thread([this]() {
				this->BackendThread();
			}
		);
	}

	RuntimeVirtualTextureBackend::~RuntimeVirtualTextureBackend() {
		mThreadRunning = false;
	}

	void RuntimeVirtualTextureBackend::BackendThread() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* mainRenderFrameFence = renderEngine->mRenderFrameFence.get();
		uint64_t previousMainFrameFenceValue = 0u;

		while (mThreadRunning) {
			uint64_t currentMainFrameFenceValue = mainRenderFrameFence->CompletedValue();

			// �µ�����Ⱦ֡���
			if (previousMainFrameFenceValue != currentMainFrameFenceValue) {
				previousMainFrameFenceValue = currentMainFrameFenceValue;

				std::vector<RuntimeVirtualTextureNodeRequestTask> rvtNodeRequestTasks;
				ProcessTerrainFeedback(rvtNodeRequestTasks, currentMainFrameFenceValue);

				if (!rvtNodeRequestTasks.empty()) {
					// ѹ���µ�RvtFrame
					uint64_t currFrameFenceExpectedValue = mRvtGraphicsFence->IncrementExpectedValue();
					mRvtFrameTracker->PushCurrentFrame(currFrameFenceExpectedValue);

					// ��¼��ǰ֡��Ӧ��Gpu����
					RecordedGpuCommand recordedGpuCommand{};

					// ���ݵ�ǰ��������¼��GPU����
					RecordGpuCommand(rvtNodeRequestTasks, recordedGpuCommand);

					// Ԥ�����������Ա����֡��ɺ�Ļص�����(mBackComputeFrameTracker->GetFrameIndex()������Ҳ����)
					mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).insert(mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).end(), rvtNodeRequestTasks.begin(), rvtNodeRequestTasks.end());

					// ������ѹ�������
					mRecordedGpuCommands.Push(std::move(recordedGpuCommand));
				}
			}

			// ���̼߳�⵽RvtPageTable��ViewRect�����䶯�������ñ䶯֪ͨ��RvtBackend�̣߳���ʱ�����߳�ͬ���ȴ�RvtBackend�̵߳Ĵ���
			if (mRenderer->mRvtPageTableViewChangedFlag == 1u) {
				--mRenderer->mRvtPageTableViewChangedFlag;

				// ��InQueue InLoading InTexture��PageTableNode���д���
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

	void RuntimeVirtualTextureBackend::ProcessTerrainFeedback(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks, uint32_t completedFenceValue) {
		auto& queuedFeedbackReadbacks = mRenderer->mQueuedFeedbackReadbacks;
		auto& terrainFeedbackReadbackBuffers = mRenderer->mTerrainFeedbackReadbackBuffers;

		// Ѱ�������ʵ�Feedback
		bool feedbackFound = false;
		uint64_t latestFeedbackFenceValue = 0;
		uint32_t targetFeedbackIndex{ 0u };
		for (size_t i = 0; i < queuedFeedbackReadbacks.size(); i++) {
			if (queuedFeedbackReadbacks.at(i).isFresh) {
				uint64_t feedbackFenceValue = queuedFeedbackReadbacks.at(i).renderFrameFenceValue;
				if ((completedFenceValue >= feedbackFenceValue) &&
					((!feedbackFound) || (latestFeedbackFenceValue <= feedbackFenceValue))) {
					feedbackFound = true;
					targetFeedbackIndex = i;
					latestFeedbackFenceValue = feedbackFenceValue;
					// this feedback will either be used or skipped. either way it is "consumed"
					queuedFeedbackReadbacks.at(i).isFresh = false;
				}
			}
		}

		if (!feedbackFound) {
			return;
		}
;
		auto& readbackBuffer = terrainFeedbackReadbackBuffers.at(targetFeedbackIndex);
		uint16_t* pResolvedData = reinterpret_cast<uint16_t*>(readbackBuffer->Map());

		auto& feedbackMapDesc = mRenderer->mTerrainFeedbackMap->GetResourceFormat().GetTextureDesc();
		uint32_t width = feedbackMapDesc.width;
		uint32_t height = feedbackMapDesc.height;

		// rowByteSize�ĵ�λ��Byte����������2Byte��ȡ�ģ����Ҫ����2
		uint32_t rowByteSize = (width * GHL::GetFormatStride(feedbackMapDesc.format) + 0x0ff) & ~0x0ff;
		rowByteSize /= (sizeof(uint16_t) / sizeof(uint8_t));

		auto* tileCache = mRenderer->mNearTerrainRuntimeVirtualTextureAtlasTileCache.get();

		for (uint32_t y = 0u; y < height; y++) {
			for (uint32_t x = 0u; x < rowByteSize;) {

				uint16_t page0PosX = pResolvedData[x++];	// page0PosX
				uint16_t page0PosY = pResolvedData[x++];	// page0PosY
				uint16_t pageLevel = pResolvedData[x++];	// pageLevel
				uint16_t overBound = pResolvedData[x++];	// overBound

				if (overBound == 0u) { continue; }

				// ����������(ע���޳��ظ�Ŀ��)
				auto& currNodeRuntimeState = mRvtPageTables.at(pageLevel).GetNodeRuntimeState(page0PosX, page0PosY);

				if (currNodeRuntimeState.inReady || currNodeRuntimeState.inQueue || currNodeRuntimeState.inLoading) {
					// �ýڵ��Ӧ����Դ���ڼ���
					continue;
				}
				// �ýڵ��Ӧ����Դ����ͼ����
				else if (currNodeRuntimeState.inTexture) {
					tileCache->Remove(currNodeRuntimeState.atlasNode);
					tileCache->AddTail(currNodeRuntimeState.atlasNode);
				}
				else {
					// ������Ӧ�ĵ�����������(��������ͼ��Ԫ��)
					RuntimeVirtualTextureNodeRequestTask requestTask{};
					requestTask.nextPageLevel = pageLevel;
					requestTask.nextPagePos.x = page0PosX;
					requestTask.nextPagePos.y = page0PosY;
					requestTasks.push_back(requestTask);

					// �޳��ظ�requestTask
					currNodeRuntimeState.SetInReady();
				}
			}

			// ת�Ƶ��������һ�У������ÿһ�г������һ�ж���Ҫ256�ֽڶ���
			pResolvedData += rowByteSize;
		}
		readbackBuffer->UnMap();

		// �ָ�PageTableNodeʵʱ״̬
		for (auto& requestTask : requestTasks) {
			auto& currNodeRuntimeState = mRvtPageTables.at(requestTask.nextPageLevel).GetNodeRuntimeState(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInActive();
		}

		// ��װpageLevel�Ӵ�С��������н�������
		std::sort(requestTasks.begin(), requestTasks.end(),
			[](const RuntimeVirtualTextureNodeRequestTask& taskA, const RuntimeVirtualTextureNodeRequestTask& taskB) {
				return taskA.nextPageLevel < taskB.nextPageLevel;
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
				requestTask.nextPagePos = atlasNode->pagePos;
				requestTask.nextPageLevel = atlasNode->pageLevel;
			}
			// ʹ�õ�atlasNodeû�и��ص��νڵ�
			else {
				requestTask.prevPagePos = Math::Int2{ -1, -1 };
				requestTask.prevPageLevel = -1;
			}

			requestTask.atlasNode = atlasNode;

			auto& currNodeRuntimeState = mRvtPageTables.at(requestTask.nextPageLevel).GetNodeRuntimeState(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInReady();
		}
	}

	void RuntimeVirtualTextureBackend::RecordGpuCommand(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand) {

	}

	void RuntimeVirtualTextureBackend::SetupFrameCompletedCallBack() {
		// ���FrameCompletedCallback
		mRvtFrameTracker->AddFrameCompletedCallBack([this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
			this->OnFrameCompleted(attribute.frameIndex);
		});
	}

	void RuntimeVirtualTextureBackend::OnFrameCompleted(uint8_t frameIndex) {
		auto* rvtTextureAtlasTileCache = mRenderer->mNearTerrainRuntimeVirtualTextureAtlasTileCache.get();

		auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks.at(frameIndex);
		for (auto& requestTask : reservedRequestTasks) {
			// ���½ڵ�ʵʱ״̬
			auto& currNodeRuntimeState = mRvtPageTables.at(requestTask.nextPageLevel).GetNodeRuntimeState(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInTexture();
			currNodeRuntimeState.atlasNode = requestTask.atlasNode;

			// ����AtlasNode����
			requestTask.atlasNode->pageLevel = requestTask.nextPageLevel;
			requestTask.atlasNode->pagePos = requestTask.nextPagePos;
			rvtTextureAtlasTileCache->AddTail(requestTask.atlasNode);
		}
		reservedRequestTasks.clear();
	}

	void RuntimeVirtualTextureBackend::CreateGraphicsObject() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageFactory = renderEngine->mDStorageFactory.get();

		auto  shaderPath = renderEngine->smEngineShaderPath;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();

		// ����ͼ��API�������ûص�����
		{
			mRvtResourceStateTracker = std::make_unique<Renderer::ResourceStateTracker>();

			mRvtGraphicsQueue = std::make_unique<GHL::CopyQueue>(device);
			mRvtGraphicsQueue->SetDebugName("RuntimeVT_GraphicsQueue");
			mRvtGraphicsFence = std::make_unique<GHL::Fence>(device);
			mRvtGraphicsFence->SetDebugName("RuntimeVT_GraphicsFence");
			mRvtFrameTracker = std::make_unique<Renderer::RingFrameTracker>(smMaxRvtFrameCount);
			mRvtLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mRvtFrameTracker.get());
			mRvtCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mRvtFrameTracker.get());
		}

		{

		}

		// ����֡��ɻص�����
		SetupFrameCompletedCallBack();
	}

}