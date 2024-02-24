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

		// 启动线程
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

			// 新的主渲染帧完成
			if (previousMainFrameFenceValue != currentMainFrameFenceValue) {
				previousMainFrameFenceValue = currentMainFrameFenceValue;

				std::vector<RuntimeVirtualTextureNodeRequestTask> rvtNodeRequestTasks;
				ProcessTerrainFeedback(rvtNodeRequestTasks, currentMainFrameFenceValue);

				if (!rvtNodeRequestTasks.empty()) {
					// 压入新的RvtFrame
					uint64_t currFrameFenceExpectedValue = mRvtGraphicsFence->IncrementExpectedValue();
					mRvtFrameTracker->PushCurrentFrame(currFrameFenceExpectedValue);

					// 记录当前帧对应的Gpu命令
					RecordedGpuCommand recordedGpuCommand{};

					// 根据当前请求任务录制GPU命令
					RecordGpuCommand(rvtNodeRequestTasks, recordedGpuCommand);

					// 预留请求任务，以便后续帧完成后的回调处理(mBackComputeFrameTracker->GetFrameIndex()理论上也可以)
					mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).insert(mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).end(), rvtNodeRequestTasks.begin(), rvtNodeRequestTasks.end());

					// 将命令压入队列中
					mRecordedGpuCommands.Push(std::move(recordedGpuCommand));
				}
			}

			// 主线程检测到RvtPageTable的ViewRect发生变动，并将该变动通知给RvtBackend线程，此时，主线程同步等待RvtBackend线程的处理
			if (mRenderer->mRvtPageTableViewChangedFlag == 1u) {
				--mRenderer->mRvtPageTableViewChangedFlag;

				// 对InQueue InLoading InTexture的PageTableNode进行处理
			}

			// 检测塞入的GPU命令是否过载
			if (mRvtFrameTracker->GetUsedSize() == smMaxRvtFrameCount) {
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

				UINT64 valueToWaitFor = mRvtGraphicsFence->ExpectedValue() - (smMaxRvtFrameCount - 1u);
				mRvtGraphicsFence->SetCompletionEvent(valueToWaitFor, eventHandle);

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}

			// 检测并处理渲染帧是否完成
			mRvtFrameTracker->PopCompletedFrame(mRvtGraphicsFence->CompletedValue());

			if (!mThreadRunning) {
				break;
			}
		}
	}

	void RuntimeVirtualTextureBackend::ProcessTerrainFeedback(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks, uint32_t completedFenceValue) {
		auto& queuedFeedbackReadbacks = mRenderer->mQueuedFeedbackReadbacks;
		auto& terrainFeedbackReadbackBuffers = mRenderer->mTerrainFeedbackReadbackBuffers;

		// 寻找最新鲜的Feedback
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

		// rowByteSize的单位是Byte，而我们是2Byte读取的，因此要除以2
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

				// 填充请求队列(注意剔除重复目标)
				auto& currNodeRuntimeState = mRvtPageTables.at(pageLevel).GetNodeRuntimeState(page0PosX, page0PosY);

				if (currNodeRuntimeState.inReady || currNodeRuntimeState.inQueue || currNodeRuntimeState.inLoading) {
					// 该节点对应的资源正在加载
					continue;
				}
				// 该节点对应的资源已在图集上
				else if (currNodeRuntimeState.inTexture) {
					tileCache->Remove(currNodeRuntimeState.atlasNode);
					tileCache->AddTail(currNodeRuntimeState.atlasNode);
				}
				else {
					// 创建对应的地形请求任务(但不分配图集元素)
					RuntimeVirtualTextureNodeRequestTask requestTask{};
					requestTask.nextPageLevel = pageLevel;
					requestTask.nextPagePos.x = page0PosX;
					requestTask.nextPagePos.y = page0PosY;
					requestTasks.push_back(requestTask);

					// 剔除重复requestTask
					currNodeRuntimeState.SetInReady();
				}
			}

			// 转移到纹理的下一行，纹理的每一行除了最后一行都需要256字节对齐
			pResolvedData += rowByteSize;
		}
		readbackBuffer->UnMap();

		// 恢复PageTableNode实时状态
		for (auto& requestTask : requestTasks) {
			auto& currNodeRuntimeState = mRvtPageTables.at(requestTask.nextPageLevel).GetNodeRuntimeState(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInActive();
		}

		// 安装pageLevel从大到小对请求队列进行排序
		std::sort(requestTasks.begin(), requestTasks.end(),
			[](const RuntimeVirtualTextureNodeRequestTask& taskA, const RuntimeVirtualTextureNodeRequestTask& taskB) {
				return taskA.nextPageLevel < taskB.nextPageLevel;
			}
		);

		// 对请求数量进行限制
		if (requestTasks.size() > mTerrainSetting.smRvtDataLoadedLimit) {
			requestTasks.resize(mTerrainSetting.smRvtDataLoadedLimit);
		}

		// 分配Tile
		for (auto& requestTask : requestTasks) {
			auto* atlasNode = tileCache->GetHead();
			tileCache->Remove(atlasNode);

			// 如果该atlasNode已经负载了一个地形节点的资源，做记录
			if (atlasNode->pageLevel != -1) {
				requestTask.nextPagePos = atlasNode->pagePos;
				requestTask.nextPageLevel = atlasNode->pageLevel;
			}
			// 使用的atlasNode没有负载地形节点
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
		// 添加FrameCompletedCallback
		mRvtFrameTracker->AddFrameCompletedCallBack([this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
			this->OnFrameCompleted(attribute.frameIndex);
		});
	}

	void RuntimeVirtualTextureBackend::OnFrameCompleted(uint8_t frameIndex) {
		auto* rvtTextureAtlasTileCache = mRenderer->mNearTerrainRuntimeVirtualTextureAtlasTileCache.get();

		auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks.at(frameIndex);
		for (auto& requestTask : reservedRequestTasks) {
			// 更新节点实时状态
			auto& currNodeRuntimeState = mRvtPageTables.at(requestTask.nextPageLevel).GetNodeRuntimeState(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInTexture();
			currNodeRuntimeState.atlasNode = requestTask.atlasNode;

			// 更新AtlasNode负载
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

		// 创建图形API对象并设置回调函数
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

		// 设置帧完成回调函数
		SetupFrameCompletedCallBack();
	}

}