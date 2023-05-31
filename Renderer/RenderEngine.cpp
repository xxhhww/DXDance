#include "RenderEngine.h"
#include "GBufferPass.h"
#include "DeferredLightPass.h"
#include "BackBufferPass.h"

#include "ECS/Entity.h"

namespace Renderer {

	RenderEngine::RenderEngine(HWND windowHandle, uint64_t width, uint64_t height, uint8_t numBackBuffers)
		: mWindowHandle(windowHandle)
		, mOutputWidth(width)
		, mOutputHeight(height)
		, mBackBufferStrategy((GHL::BackBufferStrategy)numBackBuffers)
		, mAdapterContainer(std::make_unique<GHL::AdapterContainer>())
		, mSelectedAdapter(mAdapterContainer->GetHighPerformanceAdapter())
		, mDevice(std::make_unique<GHL::Device>(*mSelectedAdapter, false))
		, mUploaderEngine(std::make_unique<UploaderEngine>(mDevice.get()))
		, mGraphicsQueue(std::make_unique<GHL::GraphicsQueue>(mDevice.get()))
		, mComputeQueue(std::make_unique<GHL::ComputeQueue>(mDevice.get()))
		, mCopyQueue(std::make_unique<GHL::CopyQueue>(mDevice.get()))
		, mRenderFrameFence(std::make_unique<GHL::Fence>(mDevice.get()))
		, mFrameTracker(std::make_unique<RingFrameTracker>(numBackBuffers))
		, mHeapAllocator(std::make_unique<BuddyHeapAllocator>(mDevice.get(), mFrameTracker.get()))
		, mCommandListAllocator(std::make_unique<PoolCommandListAllocator>(mDevice.get(), mFrameTracker.get()))
		, mDescriptorAllocator(std::make_unique<PoolDescriptorAllocator>(mDevice.get(), mFrameTracker.get(), std::vector<uint64_t>{1024, 128, 128, 128}))
		, mSharedMemAllocator(std::make_unique<LinearBufferAllocator>(mDevice.get(), mFrameTracker.get()))
		, mGPUProfiler(std::make_unique<GPUProfiler>(mDevice.get(), mFrameTracker.get()))
		, mShaderManger(std::make_unique<ShaderManger>(mDevice.get()))
		, mCommandSignatureManger(std::make_unique<CommandSignatureManger>(mDevice.get()))
		, mResourceStateTracker(std::make_unique<ResourceStateTracker>())
		, mStreamTextureManger(std::make_unique<StreamTextureManger>(
			mDevice.get(),
			mCopyQueue.get(),
			mDescriptorAllocator.get(),
			mHeapAllocator.get(),
			mFrameTracker.get(),
			mUploaderEngine->GetDSFactory(),
			mUploaderEngine->GetFileCopyQueue(),
			mUploaderEngine->GetMemoryCopyQueue()))
		, mRenderGraph(std::make_unique<RenderGraph>(
			mDevice.get(),
			mFrameTracker.get(), 
			mDescriptorAllocator.get(), 
			mCommandListAllocator.get(), 
			mGraphicsQueue.get(), 
			mComputeQueue.get(), 
			mCopyQueue.get(), 
			mResourceStateTracker.get(),
			mShaderManger.get(),
			mCommandSignatureManger.get(),
			mSharedMemAllocator.get(),
			mStreamTextureManger.get())) 
		, mPipelineResourceStorage(mRenderGraph->GetPipelineResourceStorage()) {

		if (windowHandle != nullptr) {
			mSwapChain = std::make_unique<GHL::SwapChain>(&mSelectedAdapter->GetDisplay(), mGraphicsQueue->D3DCommandQueue(), windowHandle, mBackBufferStrategy, width, height);
			for (uint32_t i = 0; i < numBackBuffers; i++) {
				mBackBuffers.emplace_back(std::make_unique<Texture>(mDevice.get(), mSwapChain->D3DBackBuffer(i), mDescriptorAllocator.get()));
			}
		}

		// 创建FinalOutput纹理
		TextureDesc _FinalOutputDesc{};
		_FinalOutputDesc.width = width;
		_FinalOutputDesc.height = height;
		_FinalOutputDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		_FinalOutputDesc.expectedState |= (GHL::EResourceState::PixelShaderAccess | GHL::EResourceState::RenderTarget);
		_FinalOutputDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		mFinalOutput = std::make_unique<Texture>(mDevice.get(), ResourceFormat{ mDevice.get(), _FinalOutputDesc }, mDescriptorAllocator.get(), nullptr);
		mFinalOutputID = mRenderGraph->ImportResource("FinalOutput", mFinalOutput.get());

		mResourceStateTracker->StartTracking(mFinalOutput.get());

		// 添加RenderPass
		mTerrainPass.AddPass(*mRenderGraph.get());
		mBackBufferPass.AddPass(*mRenderGraph.get());

		mRenderGraph->Build();

		mStreamTextureManger->Request("E:/MyProject/DXDance/Renderer/media/4ktiles.xet");
	}

	void RenderEngine::Resize(uint64_t width, uint64_t height) {

	}

	void RenderEngine::Update(float dt, const ECS::Camera& editorCamera, const ECS::Transform& cameraTransform) {
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.position = cameraTransform.worldPosition;
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.view = editorCamera.viewMatrix.Transpose();
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.projection = editorCamera.projMatrix.Transpose();
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.viewProjection = (editorCamera.viewMatrix * editorCamera.projMatrix).Transpose();

		ECS::Entity::Foreach([&](ECS::Camera& camera, ECS::Transform& transform) {
			if (camera.mainCamera == true && camera.cameraType == ECS::CameraType::RenderCamera) {
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.position = transform.worldPosition;
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.view = camera.viewMatrix.Transpose();
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.projection = camera.projMatrix.Transpose();
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.viewProjection = (camera.viewMatrix * camera.projMatrix).Transpose();
			}
		});
	}

	void RenderEngine::Render() {
		// 压入新的渲染帧
		mRenderFrameFence->IncrementExpectedValue();
		mFrameTracker->PushCurrentFrame(mRenderFrameFence->ExpectedValue());

		// RootConstantsDataPerFrame
		auto dynamicAllocation = mSharedMemAllocator->Allocate(sizeof(RootConstantsPerFrame), 256u);
		memcpy(dynamicAllocation.cpuAddress, &mPipelineResourceStorage->rootConstantsPerFrame, sizeof(RootConstantsPerFrame));
		mPipelineResourceStorage->rootConstantsPerFrameAddress = dynamicAllocation.gpuAddress;

		mRenderGraph->Execute();

		{
			RenderContext renderContext{
				mShaderManger.get(), mCommandSignatureManger.get(), mSharedMemAllocator.get(),
				mRenderGraph->GetPipelineResourceStorage(), mStreamTextureManger.get() 
			};

			CommandListWrap commandList = mCommandListAllocator->AllocateGraphicsCommandList();
			
			// 录制Editor Render Pass命令
			mEditorRenderPass.Invoke(commandList, renderContext);

			// 将FinalOutput转换为PixelShaderAccess供外部读取
			GHL::ResourceBarrierBatch barrierBatch = mResourceStateTracker->TransitionImmediately(mFinalOutput.get(), 0u, GHL::EResourceState::PixelShaderAccess);
			
			commandList->D3DCommandList()->ResourceBarrier(barrierBatch.Size(), barrierBatch.D3DBarriers());
			commandList->Close();
			mGraphicsQueue->ExecuteCommandList(commandList->D3DCommandList());
		}

		// 压入渲染命令完成后的围栏
		mGraphicsQueue->SignalFence(*mRenderFrameFence.get());

		// TODO 除了等待GPU任务的完成外，渲染主线程还需要等待ProcessThread线程的任务完成情况
		if (mFrameTracker->GetUsedSize() == std::underlying_type<GHL::BackBufferStrategy>::type(mBackBufferStrategy)) {
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

			UINT64 valueToWaitFor = mRenderFrameFence->ExpectedValue() - (std::underlying_type<GHL::BackBufferStrategy>::type(mBackBufferStrategy) - 1u);
			mRenderFrameFence->SetCompletionEvent(valueToWaitFor, eventHandle);

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}

		// 检测并处理渲染帧是否完成
		mFrameTracker->PopCompletedFrame(mRenderFrameFence->CompletedValue());
	}

	void RenderEngine::BindFinalOuputSRV(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
		mFinalOutput->BindSRDescriptor(cpuHandle);
	}

}