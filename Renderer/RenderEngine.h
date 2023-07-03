#pragma once
#include "GHL/SwapChain.h"
#include "GHL/Device.h"
#include "GHL/Adapter.h"
#include "GHL/AdapterContainer.h"
#include "GHL/Display.h"
#include "GHL/CommandQueue.h"

#include "ECS/CCamera.h"
#include "ECS/CTransform.h"

#include "Renderer/UploaderEngine.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/GPUProfiler.h"
#include "Renderer/ShaderManger.h"
#include "Renderer/StreamTextureManger.h"
#include "Renderer/CommandSignatureManger.h"

#include "Renderer/RenderGraph.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/RootConstantsPerFrame.h"

#include "Renderer/GBufferPass.h"
#include "Renderer/TerrainPass.h"
#include "Renderer/RngSeedGenerationPass.h"
#include "Renderer/SkyGenerationPass.h"
#include "Renderer/DeferredLightPass.h"
#include "Renderer/VolumetricCloudsPass.h"
#include "Renderer/TAAPass.h"
#include "Renderer/ToneMappingPass.h"
#include "Renderer/FinalBarrierPass.h"

#include "Renderer/Mesh.h"

#include "Tools/Event.h"

#include <memory>

namespace Renderer {
	/*
	* ÿһ����Ⱦ֡�������´���
	* Push�µ�֡ --- ���裬����RenderGraph --- ¼��֡���� --- �ύ֡���� --- ���֡���е������� --- ֪ͨ������֡�������Ĺ�����(��GPUProfiler TileUpdateManger��) --- ������һ֡(������Ҫ�ȴ�GPU)
	*/
	class RenderEngine {
	public:
		RenderEngine(HWND windowHandle, uint64_t width, uint64_t height, uint8_t numBackBuffers = 3u);
		RenderEngine(const RenderEngine& other) = delete;
		RenderEngine(RenderEngine&& other) = default;

		RenderEngine& operator=(const RenderEngine& other) = delete;
		RenderEngine& operator=(RenderEngine&& other) = default;

		~RenderEngine();

		void Resize(uint64_t width, uint64_t height);

		/*
		* ����RootConstantsPerFrame
		*/
		void Update(float dt, const ECS::Camera& editorCamera, const ECS::Transform& cameraTransform);

		void Render();

		void DoOfflineTask();

		void BindFinalOuputSRV(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

	private:
		/*
		* ����Sky(����Sun Light)
		*/
		void UpdateSky();

		/*
		* ����Lights(������Sun Light)
		*/
		void UpdateLights();

	public:
		// ==========================...����...==========================
		HWND mWindowHandle{ nullptr };
		uint32_t mOutputWidth{ 0u };
		uint32_t mOutputHeight{ 0u };
		GHL::BackBufferStrategy mBackBufferStrategy{ GHL::BackBufferStrategy::Triple };

		// ==========================...GPU�豸...==========================
		std::unique_ptr<GHL::AdapterContainer> mAdapterContainer; // ����������
		const GHL::Adapter* mSelectedAdapter; // ѡ��ĸ�����������
		const GHL::Display* mSelectedDisplay; // ��ʾ��
		std::unique_ptr<GHL::Device> mDevice;

		// ==========================...UploadEngine...==========================
		std::unique_ptr<UploaderEngine> mUploaderEngine;

		std::unique_ptr<GHL::GraphicsQueue>    mGraphicsQueue; // ͼ������
		std::unique_ptr<GHL::ComputeQueue>     mComputeQueue;  // ��������
		std::unique_ptr<GHL::CopyQueue>        mCopyQueue;     // ��������

		std::unique_ptr<GHL::SwapChain> mSwapChain; // ������(RenderEngine������Ҫ������������������һ��ͼƬ�����ϲ�ģ��ʹ��)
		std::vector<std::unique_ptr<Texture>> mBackBuffers;  // �󻺳�

		std::unique_ptr<GHL::Fence> mRenderFrameFence; // ��Ⱦ֡Χ��

		// ==========================...֡״̬���������ػ��������������...==========================

		//Tool::Event<> mPreRenderEvent;  // ��Ⱦ֡��ʼǰ�������¼�
		//Tool::Event<> mPostRenderEvent; // ��Ⱦ֡�����󴥷����¼�
		
		std::unique_ptr<RingFrameTracker> mFrameTracker; // ֡״̬������(ÿ֡��ʼʱѹ���µ�֡״̬ ÿ֡����ʱ���֡״̬�����е�������)

		std::unique_ptr<BuddyHeapAllocator>        mHeapAllocator;        // �ػ��Ļ��ѷ�����
		std::unique_ptr<PoolCommandListAllocator>  mCommandListAllocator; // �ػ��������б������
		std::unique_ptr<PoolDescriptorAllocator>   mDescriptorAllocator;  // �ػ���������������
		std::unique_ptr<ResourceAllocator>         mResourceAllocator;    // �ػ�����Դ������
		std::unique_ptr<LinearBufferAllocator>     mSharedMemAllocator;   // ���ԵĹ����ڴ������(���ڷ���ConstantBuffer ShaderBuffer)

		std::unique_ptr<GPUProfiler>			mGPUProfiler;			// Query RenderPass Data
		std::unique_ptr<ShaderManger>			mShaderManger;			// ��ɫ��������
		std::unique_ptr<CommandSignatureManger> mCommandSignatureManger;// ����ǩ��������
		std::unique_ptr<ResourceStateTracker>	mResourceStateTracker;	// ��Դ״̬׷����
		std::unique_ptr<StreamTextureManger>    mStreamTextureManger;	// ��ʽ���������
		// ==========================...RenderGraph...==========================
		std::unique_ptr<RenderGraph> mRenderGraph;

		// ==========================...RenderPasses...==========================
		GBufferPass				mGBufferPass;
		TerrainPass				mTerrainPass;
		RngSeedGenerationPass	mRngSeedGenerationPass;
		SkyGenerationPass		mSkyGenerationPass;
		DeferredLightPass		mDeferredLightPass;
		VolumetricCloudsPass    mVolumetricCloudsPass;
		TAAPass					mTAAPass;
		ToneMappingPass			mToneMappingPass;
		FinalBarrierPass		mFinalBarrierPass;

		// ==========================...PipelineResources...==========================
		std::unique_ptr<Texture> mFinalOutput;
		RenderGraphResourceID mFinalOutputID;

		TextureWrap mBlueNoise2DMap;
		RenderGraphResourceID mBlueNoise2DMapID;
		
		TextureWrap mBlueNoise3DMap;
		RenderGraphResourceID mBlueNoise3DMapID;

		RenderGraphResourceStorage* mPipelineResourceStorage{ nullptr };

		// ==========================...Editor Render Pass...==========================
		// �༭����ȾPass�����ⲿע��
		Tool::Event<CommandBuffer&, RenderContext&> mEditorRenderPass;

		// ==========================...For Output BackBuffer Pass...==========================
		std::unique_ptr<Renderer::Mesh> mOutputQuadMesh;

		// ==========================...Offline Task Pass...==========================
		// ��Ⱦ�������������ⲿע��
		std::unique_ptr<GHL::Fence> mOfflineFence;	// ��������դ��
		Tool::Event<CommandBuffer&, RenderContext&> mOfflineTaskPass;
		Tool::Event<> mOfflineCompletedCallback;
	};
}