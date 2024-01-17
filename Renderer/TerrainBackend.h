#pragma once
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceStateTracker.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"

#include "Renderer/TerrainTextureAtlasTileCache.h"

#include "GHL/DirectStorageQueue.h"


namespace Renderer {

	/*
	* 地形资源调度请求
	*/
	struct TerrainNodeRequestTask {
	public:
		TerrainTextureAtlasTileCache::Node* atlasNode{ nullptr };	// 一个图集元素
		int32_t prevTerrainNodeIndex{ -1 };		// 前任地形节点索引
		int32_t nextTerrainNodeIndex{ -1 };		// 下任地形节点索引
	};

	/*
	* 描述如何更新地形节点描述的结构体
	*/
	struct GpuUpdateTerrainNodeDescriptorRequest {
	public:
		uint32_t dstTerrainNodeIndex;					// 目标地形节点索引

		TerrainNodeDescriptor terrainNodeDescriptor;	// 目标地形节点新的描述
	};

	/*
	* 后台类(负责创建与管理地形节点的全量表、流入流出地形节点数据、并与GPU驻留的地形数据做交互)
	*/
	class TerrainBackend {
	public:
		TerrainBackend(
			TerrainRenderer* renderer,
			TerrainSetting& terrainSetting,
			std::vector<TerrainLodDescriptor>&  terrainLodDescriptors,
			std::vector<TerrainNodeDescriptor>& terrainNodeDescriptors,
			std::vector<TerrainNodeRuntimeState>& terrainNodeRuntimeStates);

		~TerrainBackend();

		// 预加载
		void Preload();

	private:
		// 后台线程
		void BackendThread();

		void CreateGraphicsObject();

	private:
		TerrainRenderer* mRenderer{ nullptr };

		inline static uint32_t smMaxBackFrameCount = 3u;

		std::unique_ptr<GHL::DirectStorageQueue> mBackDStorageQueue;
		std::unique_ptr<GHL::Fence>			     mBackDStorageFence;

		std::unique_ptr<GHL::CommandQueue>       mBackCopyQueue;
		std::unique_ptr<GHL::Fence>              mBackCopyFence;

		std::unique_ptr<GHL::CommandQueue>       mBackGrahpicsQueue;
		std::unique_ptr<GHL::Fence>		         mBackGraphicsFence;

		std::unique_ptr<GHL::CommandQueue>       mBackComputeQueue;
		std::unique_ptr<GHL::Fence>              mBackComputeFence;

		std::unique_ptr<Renderer::RingFrameTracker>         mBackFrameTracker;
		std::unique_ptr<Renderer::ResourceStateTracker>     mBackResourceStateTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mBackLinearBufferAllocator;		// 分配普通的PassData
		std::unique_ptr<Renderer::PoolCommandListAllocator> mBackPoolCommandListAllocator;

		std::unique_ptr<Renderer::LinearBufferAllocator>    mBackTempLinearBufferAllocator;		// 纹理Tile在共享内存中的TempBuffer

		// 线程同步变量
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// 地形数据(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
		std::vector<TerrainLodDescriptor>&    mTerrainLodDescriptors;		// 地形全LOD内容描述表
		std::vector<TerrainNodeDescriptor>&   mTerrainNodeDescriptors;		// 地形全节点内容描述表
		std::vector<TerrainNodeRuntimeState>& mTerrainNodeRuntimeStates;	// 地形全节点运行时状态

		struct UpdateTerrainNodeDescriptorPassData {
		public:
			uint32_t terrainNodeDescriptorBufferIndex;
			uint32_t updateTerrainNodeDescriptorRequestBufferIndex;
		};
		UpdateTerrainNodeDescriptorPassData mUpdateTerrainNodeDescriptorPassData;
		BufferWrap mUpdateTerrainNodeDescriptorRequestBuffer;	// 用于更新地形节点描述的缓冲

		// 着色器程序名称
		inline static std::string smUpdateTerrainNodeDescriptorSN = "UpdateTerrainNodeDescriptor";
		inline static uint32_t smThreadSizeInGroup = 8u;
	};

}