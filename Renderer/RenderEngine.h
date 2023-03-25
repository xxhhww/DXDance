#pragma once
#include "GHL/SwapChain.h"
#include "GHL/Device.h"
#include "GHL/Adapter.h"
#include "GHL/AdapterContainer.h"
#include "GHL/CommandQueue.h"

#include "RingFrameTracker.h"

#include "BuddyHeapAllocator.h"
#include "PoolDescriptorAllocator.h"
#include "PoolCommandListAllocator.h"
#include "LinearBufferAllocator.h"

#include "GPUProfiler.h"
#include "ShaderManger.h"
#include "TileUpdateManger.h"

#include "Tools/Event.h"

#include <memory>

namespace Renderer {

	/*
	* ÿһ����Ⱦ֡�������´���
	* Push�µ�֡ --- ���裬����RenderGraph --- ¼��֡���� --- �ύ֡���� --- ���֡���е������� --- ֪ͨ������֡�������Ĺ�����(��GPUProfiler TileUpdateManger��) --- ������һ֡(������Ҫ�ȴ�GPU)
	*/
	class RenderEngine {
	public:
		RenderEngine();
		RenderEngine(const RenderEngine& other) = delete;
		RenderEngine(RenderEngine&& other) = default;

		RenderEngine& operator=(const RenderEngine& other) = delete;
		RenderEngine& operator=(RenderEngine&& other) = default;

		~RenderEngine() = default;

		void Render();

	private:
		// ==========================...GPU�豸...==========================
		
		std::unique_ptr<GHL::AdapterContainer> mAdapterContainer; // ����������
		GHL::Adapter* mSelectedAdapter; // ѡ��ĸ�����������
		
		std::unique_ptr<GHL::Device> mDevice;

		std::unique_ptr<GHL::GraphicsQueue>    mGraphicsQueue; // ͼ������
		std::unique_ptr<GHL::ComputeQueue>     mComputeQueue;  // ��������
		std::unique_ptr<GHL::CopyQueue>        mCopyQueue;     // ��������

		std::unique_ptr<GHL::SwapChain> mSwapChain; // ������
		std::vector<ID3D12Resource*> mBackBuffers;  // �󻺳�

		std::unique_ptr<GHL::Fence> mRenderFrameFence; // ��Ⱦ֡Χ��

		// ==========================...֡״̬���������ػ��������������...==========================

		Tool::Event<> mPreRenderEvent;  // ��Ⱦ֡��ʼǰ�������¼�
		Tool::Event<> mPostRenderEvent; // ��Ⱦ֡�����󴥷����¼�
		
		std::unique_ptr<RingFrameTracker> mFrameTracker; // ֡״̬������(ÿ֡��ʼʱѹ���µ�֡״̬ ÿ֡����ʱ���֡״̬�����е�������)

		std::unique_ptr<BuddyHeapAllocator>        mHeapAllocator;        // �ػ��Ļ��ѷ�����
		std::unique_ptr<PoolCommandListAllocator>  mCommandListAllocator; // �ػ��������б������
		std::unique_ptr<PoolDescriptorAllocator>   mDescriptorAllocator;  // �ػ���������������
		std::unique_ptr<LinearBufferAllocator>     mSharedMemAllocator;   // ���ԵĹ����ڴ������(���ڷ���ConstantBuffer ShaderBuffer)

		std::unique_ptr<GPUProfiler>      mGPUProfiler;      // Query RenderPass Data
		std::unique_ptr<ShaderManger>     mShaderManger;     // ��ɫ��������
		std::unique_ptr<TileUpdateManger> mTileUpdateManger; // ��ʽ���������

		// ==========================...RenderGraph...==========================

	};

}