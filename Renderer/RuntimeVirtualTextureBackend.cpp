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
				ProcessTerrainFeedback(rvtNodeRequestTasks);

				if (!rvtNodeRequestTasks.empty()) {
					// 压入新的RvtFrame
					uint64_t currFrameFenceExpectedValue = mRvtGraphicsFence->IncrementExpectedValue();
					mRvtFrameTracker->PushCurrentFrame(currFrameFenceExpectedValue);

					// 记录当前帧对应的Gpu命令
					RecordedGpuCommand recordedGpuCommand{};

					// 根据当前请求任务录制GPU命令
					RecordGpuCommand(rvtNodeRequestTasks, recordedGpuCommand);

					// 将命令压入队列中
					mRecordedGpuCommands.Push(std::move(recordedGpuCommand));
					// 预留请求任务，以便后续帧完成后的回调处理(mBackComputeFrameTracker->GetFrameIndex()理论上也可以)
					mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).insert(mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).end(), rvtNodeRequestTasks.begin(), rvtNodeRequestTasks.end());
				}
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

	void RuntimeVirtualTextureBackend::ProcessTerrainFeedback(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks) {

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
			auto& currNodeRuntimeState = mRvtPageTables.at(requestTask.nextPageLevel).GetNodeRuntimeState(requestTask.nextPagePos);
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