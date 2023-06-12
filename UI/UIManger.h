#pragma once
#include "Canvas.h"
#include "Setting.h"

#include "Windows/Window.h"

#include "GHL/pbh.h"

namespace GHL {
	class Device;
	class GraphicsQueue;
}

namespace UI {
	class UIManger {
	public:
		/*
		* ImGUi��Imnodes�ĳ�ʼ���������ڴ�������D3D�����(�豸���������ѡ���������)
		*/
		UIManger(UIStyle style, Windows::Window* window, GHL::Device* device, GHL::GraphicsQueue* graphicsQueue);

		/*
		* ImGui��Imnodes������������(Canvas)��������Editor�����
		*/
		~UIManger();

		/*
		* Ӧ�ö�Ӧ��ImGui���
		*/
		void ApplyStyle(UIStyle style);

		/*
		* ����UIManger�Ļ�������Editor���������
		*/
		void SetCanvas(Canvas* canvas);

		/*
		* ���ƻ���
		*/
		void Darw();

		/*
		* ��ȡ����
		*/
		inline auto* GetCanvas()           const { return mCanvas; }

		/*
		* D3D���
		*/
		inline auto* GetSRDescriptorHeap() const { return g_pd3dSrvDescHeap; }
		size_t GetSRVIncrementSize() const;

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
		IDXGIFactory2* g_DXGIFactory = NULL;
		
		GHL::Device* mDevice{ nullptr };
		GHL::GraphicsQueue* mGraphicsQueue{ nullptr };

		ID3D12DescriptorHeap* g_pd3dRtvDescHeap = NULL;
		ID3D12DescriptorHeap* g_pd3dSrvDescHeap = NULL;
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