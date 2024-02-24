#pragma once
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceStateTracker.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"
#include "Renderer/RuntimeVirtualTextureAtlasTileCache.h"
#include "Renderer/RuntimeVirtualTexturePageTable.h"

#include "Tools/ConcurrentQueue.h"

namespace Renderer {

	/*
	* 实时虚拟纹理调度请求
	*/
	struct RuntimeVirtualTextureNodeRequestTask {
	public:
		RuntimeVirtualTextureAtlasTileCache::Node* atlasNode{ nullptr };	// 一个图集元素

		// 前一任
		Math::Int2 prevPagePos{ -1, -1 };
		int32_t    prevPageLevel{ -1 };

		// 下一任
		Math::Int2 nextPagePos{ -1, -1 };
		int32_t    nextPageLevel{ -1 };
	};

	class RuntimeVirtualTextureBackend {
	public:
		struct RecordedGpuCommand {
		public:
			GHL::CommandQueue* graphicsQueue{ nullptr };
			GHL::Fence*        graphicsFence{ nullptr };
			GHL::CommandList*  graphicsCommandList{ nullptr };
			uint64_t           graphicsFenceExpectedValue{ 0u };

			GHL::CommandQueue* computeQueue{ nullptr };
			GHL::Fence*        computeFence{ nullptr };
			GHL::CommandList*  computeCommandList{ nullptr };
			uint64_t           computeFenceExpectedValue{ 0u };

			uint32_t           frameIndex{ 0u };
		};

	public:
		RuntimeVirtualTextureBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting);

		~RuntimeVirtualTextureBackend();

	private:
		// 后台线程
		void BackendThread();

		// 处理Feedback
		void ProcessTerrainFeedback(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks, uint32_t completedFenceValue);

		// 录制GPU命令
		void RecordGpuCommand(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand);

		// 创建图形对象
		void CreateGraphicsObject();

		// 设置帧完成回调函数
		void SetupFrameCompletedCallBack();

		// 帧完成回调
		void OnFrameCompleted(uint8_t frameIndex);

	private:
		TerrainRenderer* mRenderer{ nullptr };

		inline static uint32_t smMaxRvtFrameCount = 3u;

		// 线程同步变量
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// 地形数据(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
		std::vector<RuntimeVirtualTexturePageTable>& mRvtPageTables;

		std::unique_ptr<GHL::CommandQueue>                  mRvtGraphicsQueue;
		std::unique_ptr<GHL::Fence>                         mRvtGraphicsFence;
		std::unique_ptr<Renderer::RingFrameTracker>	        mRvtFrameTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mRvtLinearBufferAllocator;
		std::unique_ptr<Renderer::PoolCommandListAllocator> mRvtCommandListAllocator;

		std::unique_ptr<Renderer::ResourceStateTracker>     mRvtResourceStateTracker;

		struct UpdateVirtualTextureAtlasPassData {

		};

		struct UpdatePageTablePassData {

		};

		UpdateVirtualTextureAtlasPassData mUpdateVirtualTextureAtlasPassData;
		UpdatePageTablePassData mUpdatePageTablePassData;

		Tool::ConcurrentQueue<RecordedGpuCommand> mRecordedGpuCommands;				// 该队列由BackThread和MainThread共同访问

		std::vector<std::vector<RuntimeVirtualTextureNodeRequestTask>> mReservedTerrainNodeRequestTasks;	// 预留的地形节点请求任务，以便帧完成后的回调处理

	};

}