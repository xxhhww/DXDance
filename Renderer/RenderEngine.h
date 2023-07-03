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
	* 每一个渲染帧进行如下处理
	* Push新的帧 --- 如需，构建RenderGraph --- 录制帧命令 --- 提交帧命令 --- 检测帧队列的完成情况 --- 通知依赖于帧完成情况的管理器(如GPUProfiler TileUpdateManger等) --- 移至下一帧(可能需要等待GPU)
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
		* 更新RootConstantsPerFrame
		*/
		void Update(float dt, const ECS::Camera& editorCamera, const ECS::Transform& cameraTransform);

		void Render();

		void DoOfflineTask();

		void BindFinalOuputSRV(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle);

	private:
		/*
		* 更新Sky(包括Sun Light)
		*/
		void UpdateSky();

		/*
		* 更新Lights(不包括Sun Light)
		*/
		void UpdateLights();

	public:
		// ==========================...杂项...==========================
		HWND mWindowHandle{ nullptr };
		uint32_t mOutputWidth{ 0u };
		uint32_t mOutputHeight{ 0u };
		GHL::BackBufferStrategy mBackBufferStrategy{ GHL::BackBufferStrategy::Triple };

		// ==========================...GPU设备...==========================
		std::unique_ptr<GHL::AdapterContainer> mAdapterContainer; // 适配器容器
		const GHL::Adapter* mSelectedAdapter; // 选择的高性能适配器
		const GHL::Display* mSelectedDisplay; // 显示器
		std::unique_ptr<GHL::Device> mDevice;

		// ==========================...UploadEngine...==========================
		std::unique_ptr<UploaderEngine> mUploaderEngine;

		std::unique_ptr<GHL::GraphicsQueue>    mGraphicsQueue; // 图形引擎
		std::unique_ptr<GHL::ComputeQueue>     mComputeQueue;  // 计算引擎
		std::unique_ptr<GHL::CopyQueue>        mCopyQueue;     // 复制引擎

		std::unique_ptr<GHL::SwapChain> mSwapChain; // 交换链(RenderEngine好像不需要交换链，它最后是输出一张图片来给上层模块使用)
		std::vector<std::unique_ptr<Texture>> mBackBuffers;  // 后缓冲

		std::unique_ptr<GHL::Fence> mRenderFrameFence; // 渲染帧围栏

		// ==========================...帧状态跟踪器、池化分配器与管理器...==========================

		//Tool::Event<> mPreRenderEvent;  // 渲染帧开始前触发该事件
		//Tool::Event<> mPostRenderEvent; // 渲染帧结束后触发该事件
		
		std::unique_ptr<RingFrameTracker> mFrameTracker; // 帧状态跟踪器(每帧开始时压入新的帧状态 每帧结束时检测帧状态队列中的完成情况)

		std::unique_ptr<BuddyHeapAllocator>        mHeapAllocator;        // 池化的伙伴堆分配器
		std::unique_ptr<PoolCommandListAllocator>  mCommandListAllocator; // 池化的命令列表分配器
		std::unique_ptr<PoolDescriptorAllocator>   mDescriptorAllocator;  // 池化的描述符分配器
		std::unique_ptr<ResourceAllocator>         mResourceAllocator;    // 池化的资源分配器
		std::unique_ptr<LinearBufferAllocator>     mSharedMemAllocator;   // 线性的共享内存分配器(用于分配ConstantBuffer ShaderBuffer)

		std::unique_ptr<GPUProfiler>			mGPUProfiler;			// Query RenderPass Data
		std::unique_ptr<ShaderManger>			mShaderManger;			// 着色器管理器
		std::unique_ptr<CommandSignatureManger> mCommandSignatureManger;// 命令签名管理器
		std::unique_ptr<ResourceStateTracker>	mResourceStateTracker;	// 资源状态追踪器
		std::unique_ptr<StreamTextureManger>    mStreamTextureManger;	// 流式纹理管理器
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
		// 编辑器渲染Pass，由外部注册
		Tool::Event<CommandBuffer&, RenderContext&> mEditorRenderPass;

		// ==========================...For Output BackBuffer Pass...==========================
		std::unique_ptr<Renderer::Mesh> mOutputQuadMesh;

		// ==========================...Offline Task Pass...==========================
		// 渲染器离线任务，由外部注册
		std::unique_ptr<GHL::Fence> mOfflineFence;	// 离线任务栅栏
		Tool::Event<CommandBuffer&, RenderContext&> mOfflineTaskPass;
		Tool::Event<> mOfflineCompletedCallback;
	};
}