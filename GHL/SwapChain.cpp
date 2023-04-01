#include "SwapChain.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {

	SwapChain::SwapChain(
		const Display* display,
		ID3D12CommandQueue* cmdQueue,
		HWND windowHandle,
		BackBufferStrategy strategy,
		uint32_t width,
		uint32_t height
	)
	: mColorSpace(display->colorSpace)
	, mSupportHDR(display->supportHDR)
	, mCommandQueue(cmdQueue)
	, mWindowHandle(windowHandle)
	, mStrategy(strategy)
	, mWidth(width)
	, mHeight(height) {

		HRASSERT(CreateDXGIFactory2(0, IID_PPV_ARGS(&mDXGIFactory)));

		mDesc.Width = mWidth;
		mDesc.Height = mHeight;
		mDesc.Format = GetBackBufferFormat();
		mDesc.Scaling = DXGI_SCALING_NONE;
		mDesc.SampleDesc.Count = 1;
		mDesc.SampleDesc.Quality = 0;
		mDesc.BufferUsage = DXGI_USAGE_BACK_BUFFER;
		mDesc.BufferCount = std::underlying_type<BackBufferStrategy>::type(strategy);;
		mDesc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
		mDesc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;

		mBackBuffers.resize(mDesc.BufferCount);
		CreateDXGISwapChain();

		HRASSERT(mSwapChain->SetColorSpace1(GetD3DColorSpace(mColorSpace)));

	}

	DXGI_FORMAT SwapChain::GetBackBufferFormat() const {
		switch (mColorSpace) {
		case GHL::ColorSpace::Rec709:
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		case GHL::ColorSpace::Rec2020:
			return mSupportHDR ? DXGI_FORMAT_R10G10B10A2_UNORM : DXGI_FORMAT_R8G8B8A8_UNORM;
		default:
			ASSERT_FORMAT(false, "Unsupport ColorSpace");
			return DXGI_FORMAT_R8G8B8A8_UNORM;
		}
	}

	void SwapChain::CreateDXGISwapChain() {

		Microsoft::WRL::ComPtr<IDXGISwapChain1> tempSwapChain1;
		HRASSERT(mDXGIFactory->CreateSwapChainForHwnd(mCommandQueue, mWindowHandle, &mDesc, nullptr, nullptr, &tempSwapChain1));
		HRASSERT(tempSwapChain1.As(&mSwapChain));

		for (size_t i = 0; i < mDesc.BufferCount; i++) {
			HRASSERT(mSwapChain->GetBuffer(i, IID_PPV_ARGS(&mBackBuffers.at(i))));
			std::string debugName = "BackBuffer" + std::to_string(i);
			mBackBuffers.at(i)->SetName(Tool::StrUtil::UTF8ToWString(debugName).c_str());
		}

	}

	void SwapChain::ResizeBuffer() {



	}

	void SwapChain::Present() {
		HRASSERT(mSwapChain->Present(0, 0));
	}

}