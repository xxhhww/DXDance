#pragma once
#include "Display.h"
#include <vector>

namespace GHL {

	enum class BackBufferStrategy : uint8_t {
		Double = 2, Triple = 3
	};

	/*
	* 交换链
	*/
	class SwapChain {
	public:
		SwapChain(
			const Display* display,
			ID3D12CommandQueue* cmdQueue,
			HWND windowHandle,
			BackBufferStrategy strategy,
			uint32_t width,
			uint32_t height
		);
		~SwapChain() = default;
		
		/*
		* 创建DXGI交换链
		*/
		void CreateDXGISwapChain();

		/*
		* 重置交换链
		*/
		void ResizeBuffer();

		/*
		* 
		*/
		void Present();

		/*
		* Get方法
		*/
		inline const auto  DXGISwapChain()                const { return mSwapChain.Get(); }
		inline       auto* D3DBackBuffer(uint32_t index)  const { return mBackBuffers.at(index).Get(); }
		inline const auto  D3DCommandQueue()              const { return mCommandQueue; }
		inline const auto  WinHandle()                    const { return mWindowHandle; }

		inline const auto& GetColorSpace()                const { return mColorSpace; }
		inline const auto& GetStrategy()                  const { return mStrategy; }
		inline const auto& GetWidth()                     const { return mWidth; }
		inline const auto& GetHeight()                    const { return mHeight; }

		DXGI_FORMAT GetBackBufferFormat() const;

	private:
		DXGI_SWAP_CHAIN_DESC1 mDesc{};
		ColorSpace mColorSpace = ColorSpace::Rec709;
		bool mSupportHDR{ false };
		ID3D12CommandQueue* mCommandQueue{ nullptr };
		HWND mWindowHandle;

		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mBackBuffers;
		BackBufferStrategy mStrategy;
		uint32_t mWidth;
		uint32_t mHeight;

		Microsoft::WRL::ComPtr<IDXGIFactory4> mDXGIFactory;
		Microsoft::WRL::ComPtr<IDXGISwapChain4> mSwapChain;

	};

}