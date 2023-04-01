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

#include "RenderGraph.h"

#include "Tools/Event.h"

#include <memory>

namespace Renderer {

	/*
	* 每一个渲染帧进行如下处理
	* Push新的帧 --- 如需，构建RenderGraph --- 录制帧命令 --- 提交帧命令 --- 检测帧队列的完成情况 --- 通知依赖于帧完成情况的管理器(如GPUProfiler TileUpdateManger等) --- 移至下一帧(可能需要等待GPU)
	*/
	class RenderEngine {
	public:
		RenderEngine(HWND windowHandle, uint64_t width, uint64_t height);
		RenderEngine(const RenderEngine& other) = delete;
		RenderEngine(RenderEngine&& other) = default;

		RenderEngine& operator=(const RenderEngine& other) = delete;
		RenderEngine& operator=(RenderEngine&& other) = default;

		~RenderEngine() = default;

		void Render();

	public:
		// ==========================...GPU设备...==========================
		
		std::unique_ptr<GHL::AdapterContainer> mAdapterContainer; // 适配器容器
		const GHL::Adapter* mSelectedAdapter; // 选择的高性能适配器
		
		std::unique_ptr<GHL::Device> mDevice;

		std::unique_ptr<GHL::GraphicsQueue>    mGraphicsQueue; // 图形引擎
		std::unique_ptr<GHL::ComputeQueue>     mComputeQueue;  // 计算引擎
		std::unique_ptr<GHL::CopyQueue>        mCopyQueue;     // 复制引擎

		std::unique_ptr<GHL::SwapChain> mSwapChain; // 交换链(RenderEngine好像不需要交换链，它最后是输出一张图片来给上层模块使用)
		std::vector<ID3D12Resource*> mBackBuffers;  // 后缓冲

		std::unique_ptr<GHL::Fence> mRenderFrameFence; // 渲染帧围栏

		// ==========================...帧状态跟踪器、池化分配器与管理器...==========================

		//Tool::Event<> mPreRenderEvent;  // 渲染帧开始前触发该事件
		//Tool::Event<> mPostRenderEvent; // 渲染帧结束后触发该事件
		
		std::unique_ptr<RingFrameTracker> mFrameTracker; // 帧状态跟踪器(每帧开始时压入新的帧状态 每帧结束时检测帧状态队列中的完成情况)

		std::unique_ptr<BuddyHeapAllocator>        mHeapAllocator;        // 池化的伙伴堆分配器
		std::unique_ptr<PoolCommandListAllocator>  mCommandListAllocator; // 池化的命令列表分配器
		std::unique_ptr<PoolDescriptorAllocator>   mDescriptorAllocator;  // 池化的描述符分配器
		std::unique_ptr<LinearBufferAllocator>     mSharedMemAllocator;   // 线性的共享内存分配器(用于分配ConstantBuffer ShaderBuffer)

		std::unique_ptr<GPUProfiler>      mGPUProfiler;      // Query RenderPass Data
		std::unique_ptr<ShaderManger>     mShaderManger;     // 着色器管理器
		std::unique_ptr<TileUpdateManger> mTileUpdateManger; // 流式纹理管理器

		// ==========================...RenderGraph...==========================
		std::unique_ptr<RenderGraph> mRenderGraph;
	};

}