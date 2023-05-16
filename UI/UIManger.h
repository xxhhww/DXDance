#pragma once
#include "Canvas.h"
#include "Setting.h"

#include "Windows/Window.h"

#include "GHL/pbh.h"

namespace UI {
	class UIManger {
	public:
		/*
		* ImGUi与Imnodes的初始化，依赖于窗口类与D3D相关类(设备、描述符堆、描述符等)
		*/
		UIManger(Windows::Window* window, UIStyle style);

		/*
		* ImGui与Imnodes的析构，画布(Canvas)的析构由Editor类完成
		*/
		~UIManger();

		/*
		* 应用对应的ImGui风格
		*/
		void ApplyStyle(UIStyle style);

		/*
		* 设置UIManger的画布，由Editor类进行设置
		*/
		void SetCanvas(Canvas* canvas);

		/*
		* 绘制画布
		*/
		void Darw();

		/*
		* 获取画布
		*/
		inline auto* GetCanvas()           const { return mCanvas; }

		/*
		* D3D相关
		*/
		inline auto* GetSRDescriptorHeap() const { return g_pd3dSrvDescHeap; }
		inline auto  GetSRVIncrementSize() const { return g_pd3dDevice->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV); }

	private:
		Canvas* mCanvas{ nullptr };
		UIStyle mStyle;

	// D3DHelper Function
	private:
		struct FrameContext
		{
			ID3D12CommandAllocator* CommandAllocator;
			UINT64                  FenceValue;
		};

		bool CreateDeviceD3D(HWND hWnd);
		void CleanupDeviceD3D();
		void CreateRenderTarget();
		void CleanupRenderTarget();
		void WaitForLastSubmittedFrame();
		FrameContext* WaitForNextFrameResources();

		// Data
		inline static int const	NUM_FRAMES_IN_FLIGHT = 3;
		FrameContext	g_frameContext[NUM_FRAMES_IN_FLIGHT] = {};
		UINT			g_frameIndex = 0;

		inline static int const	NUM_BACK_BUFFERS = 3;
		ID3D12Device* g_pd3dDevice = NULL;
		ID3D12DescriptorHeap* g_pd3dRtvDescHeap = NULL;
		ID3D12DescriptorHeap* g_pd3dSrvDescHeap = NULL;
		ID3D12CommandQueue* g_pd3dCommandQueue = NULL;
		ID3D12GraphicsCommandList*	g_pd3dCommandList = NULL;
		ID3D12Fence* g_fence = NULL;
		HANDLE                       g_fenceEvent = NULL;
		UINT64                       g_fenceLastSignaledValue = 0;
		IDXGISwapChain3* g_pSwapChain = NULL;
		HANDLE                       g_hSwapChainWaitableObject = NULL;
		ID3D12Resource* g_mainRenderTargetResource[NUM_BACK_BUFFERS] = {};
		D3D12_CPU_DESCRIPTOR_HANDLE  g_mainRenderTargetDescriptor[NUM_BACK_BUFFERS] = {};
	};
}