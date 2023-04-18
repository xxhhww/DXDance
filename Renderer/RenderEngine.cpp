#include "RenderEngine.h"
#include "GBufferPass.h"
#include "DeferredLightPass.h"
#include "BackBufferPass.h"

namespace Renderer {

	RenderEngine::RenderEngine(HWND windowHandle, uint64_t width, uint64_t height)
		: mAdapterContainer(std::make_unique<GHL::AdapterContainer>())
		, mSelectedAdapter(mAdapterContainer->GetHighPerformanceAdapter())
		, mDevice(std::make_unique<GHL::Device>(*mSelectedAdapter, false))
		, mGraphicsQueue(std::make_unique<GHL::GraphicsQueue>(mDevice.get()))
		, mComputeQueue(std::make_unique<GHL::ComputeQueue>(mDevice.get()))
		, mCopyQueue(std::make_unique<GHL::CopyQueue>(mDevice.get()))
		, mRenderFrameFence(std::make_unique<GHL::Fence>(mDevice.get()))
		, mFrameTracker(std::make_unique<RingFrameTracker>(3u))
		, mHeapAllocator(std::make_unique<BuddyHeapAllocator>(mDevice.get(), mFrameTracker.get()))
		, mCommandListAllocator(std::make_unique<PoolCommandListAllocator>(mDevice.get(), mFrameTracker.get()))
		, mDescriptorAllocator(std::make_unique<PoolDescriptorAllocator>(mDevice.get(), mFrameTracker.get(), std::vector<uint64_t>{1024, 128, 128, 128}))
		, mSharedMemAllocator(std::make_unique<LinearBufferAllocator>(mDevice.get(), mFrameTracker.get()))
		, mGPUProfiler(std::make_unique<GPUProfiler>(mDevice.get(), mFrameTracker.get()))
		, mShaderManger(std::make_unique<ShaderManger>(mDevice.get()))
		, mResourceStateTracker(std::make_unique<ResourceStateTracker>())
		, mRenderGraph(std::make_unique<RenderGraph>(
			mDevice.get(), mFrameTracker.get(), 
			mDescriptorAllocator.get(), 
			mCommandListAllocator.get(), 
			mGraphicsQueue.get(), 
			mComputeQueue.get(), 
			mCopyQueue.get(), 
			mResourceStateTracker.get(),
			mShaderManger.get(),
			mSharedMemAllocator.get())) {

		if (windowHandle != nullptr) {
			mSwapChain = std::make_unique<GHL::SwapChain>(&mSelectedAdapter->GetDisplay(), mGraphicsQueue->D3DCommandQueue(), windowHandle, GHL::BackBufferStrategy::Triple, width, height);
			mBackBuffers.emplace_back(std::make_unique<Texture>(mDevice.get(), mSwapChain->D3DBackBuffer(0u), mDescriptorAllocator.get()));
			mBackBuffers.emplace_back(std::make_unique<Texture>(mDevice.get(), mSwapChain->D3DBackBuffer(1u), mDescriptorAllocator.get()));
			mBackBuffers.emplace_back(std::make_unique<Texture>(mDevice.get(), mSwapChain->D3DBackBuffer(2u), mDescriptorAllocator.get()));
		}

		// 创建FinalOutput纹理
		TextureDesc _FinalOutputDesc{};
		_FinalOutputDesc.width = width;
		_FinalOutputDesc.height = height;
		_FinalOutputDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		_FinalOutputDesc.expectedState |= (GHL::EResourceState::PixelShaderAccess | GHL::EResourceState::RenderTarget);
		mFinalOutput = std::make_unique<Texture>(mDevice.get(), ResourceFormat{ mDevice.get(), _FinalOutputDesc }, mDescriptorAllocator.get(), nullptr);
		mFinalOutputID = mRenderGraph->ImportResource("FinalOutput", mFinalOutput.get());

		mResourceStateTracker->StartTracking(mFinalOutput.get());

		mBackBufferPass.AddPass(*mRenderGraph.get());

		mRenderGraph->Build();
	}

	void RenderEngine::Resize(uint64_t width, uint64_t height) {

	}

	void RenderEngine::Update(float dt) {
		// 更新Constants

	}

	void RenderEngine::Render() {
		// 压入新的渲染帧
		mRenderFrameFence->IncrementExpectedValue();
		mFrameTracker->PushCurrentFrame(mRenderFrameFence->ExpectedValue());

		mRenderGraph->Execute();

		{
			// 将FinalOutput转换为PixelShaderAccess供外部读取
			GHL::ResourceBarrierBatch barrierBatch = mResourceStateTracker->TransitionImmediately(mFinalOutput.get(), 0u, GHL::EResourceState::PixelShaderAccess);
			CommandListWrap transitionCmdList = mCommandListAllocator->AllocateGraphicsCommandList();
			transitionCmdList->D3DCommandList()->ResourceBarrier(barrierBatch.Size(), barrierBatch.D3DBarriers());
			transitionCmdList->Close();
			mGraphicsQueue->ExecuteCommandList(transitionCmdList->D3DCommandList());
		}

		// 压入渲染命令完成后的围栏
		mGraphicsQueue->SignalFence(*mRenderFrameFence.get());

		if (mFrameTracker->GetUsedSize() == 3u) {
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

			UINT64 valueToWaitFor = mRenderFrameFence->ExpectedValue() - (3u - 1u);
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