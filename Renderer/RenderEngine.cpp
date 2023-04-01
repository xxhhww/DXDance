#include "RenderEngine.h"

namespace Renderer {

	RenderEngine::RenderEngine(HWND windowHandle, uint64_t width, uint64_t height)
	: mAdapterContainer(std::make_unique<GHL::AdapterContainer>())
	, mSelectedAdapter(mAdapterContainer->GetHighPerformanceAdapter())
	, mDevice(std::make_unique<GHL::Device>(*mSelectedAdapter, false))
	, mGraphicsQueue(std::make_unique<GHL::GraphicsQueue>(mDevice.get()))
	, mComputeQueue(std::make_unique<GHL::ComputeQueue>(mDevice.get()))
	, mCopyQueue(std::make_unique<GHL::CopyQueue>(mDevice.get()))
	, mSwapChain(std::make_unique<GHL::SwapChain>(&mSelectedAdapter->GetDisplay(), mGraphicsQueue->D3DCommandQueue(), windowHandle, GHL::BackBufferStrategy::Double, width, height))
	, mRenderFrameFence(std::make_unique<GHL::Fence>(mDevice.get()))
	, mFrameTracker(std::make_unique<RingFrameTracker>(2u))
	, mHeapAllocator(std::make_unique<BuddyHeapAllocator>(mDevice.get(), mFrameTracker.get()))
	, mCommandListAllocator(std::make_unique<PoolCommandListAllocator>(mDevice.get(), mFrameTracker.get()))
	, mDescriptorAllocator(std::make_unique<PoolDescriptorAllocator>(mDevice.get(), mFrameTracker.get(), std::vector<uint64_t>{1024, 128, 128, 128}))
	, mSharedMemAllocator(std::make_unique<LinearBufferAllocator>(mDevice.get(), mFrameTracker.get()))
	, mGPUProfiler(std::make_unique<GPUProfiler>(mDevice.get(), mFrameTracker.get()))
	, mShaderManger(std::make_unique<ShaderManger>(mDevice.get())) 
	, mRenderGraph(std::make_unique<RenderGraph>(mDevice.get(), mFrameTracker.get(), mDescriptorAllocator.get())) {

		mBackBuffers.emplace_back(mSwapChain->D3DBackBuffer(0u));
		mBackBuffers.emplace_back(mSwapChain->D3DBackBuffer(1u));

	}

	void RenderEngine::Render() {

	}
}