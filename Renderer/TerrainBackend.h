#pragma once
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceStateTracker.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"
#include "Renderer/TerrainTextureAtlasTileCache.h"
#include "Renderer/TerrainTiledTextureHeapAllocationCache.h"

#include "GHL/DirectStorageQueue.h"

#include "Tools/ConcurrentQueue.h"

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
	* TerrainTiledRequestTask
	*/
	struct TerrainTiledTextureTileRequestTask {
	public:
		TerrainTiledTextureHeapAllocationCache::Node* cacheNode{ nullptr };	// 代表一个HeapAllocation
		int32_t prevTileIndex{ -1 };	// 前任Tile索引
		int32_t nextTileIndex{ -1 };	// 下任Tile索引
	};

	/*
	* 后台类(负责创建与管理地形节点的全量表、流入流出地形节点数据、并与GPU驻留的地形数据做交互)
	*/
	class TerrainBackend {
	public:
		struct RecordedGpuCommand {
		public:
			GHL::CommandQueue* copyQueue{ nullptr };
			GHL::Fence*        copyFence{ nullptr };
			GHL::CommandList*  copyCommandList{ nullptr };
			uint64_t           copyFenceExpectedValue{ 0u };

			GHL::CommandQueue* computeQueue{ nullptr };
			GHL::Fence*        computeFence{ nullptr };
			GHL::CommandList*  computeCommandList{ nullptr };
			uint64_t           computeFenceExpectedValue{ 0u };

			uint32_t           frameIndex{ 0u };
		};

	public:
		TerrainBackend(
			TerrainRenderer* renderer,
			TerrainSetting& terrainSetting,
			std::vector<TerrainLodDescriptor>&  terrainLodDescriptors,
			std::vector<TerrainNodeDescriptor>& terrainNodeDescriptors,
			std::vector<TerrainNodeRuntimeState>& terrainNodeRuntimeStates,
			std::vector<TerrainTiledTextureTileRuntimeState>& terrainTiledSplatMapTileRuntimeStates);

		~TerrainBackend();

		// 预加载
		void Preload();

		auto& GetRecordedGpuCommands() { return mRecordedGpuCommands; }

		void OnFrameLoading(uint32_t frameIndex);

	private:
		// 后台线程
		void BackendThread();

		void CreateGraphicsObject();

		// 处理地形节点请求
		void ProduceTerrainNodeRequest(std::vector<TerrainNodeRequestTask>& requestTasks, Math::Vector3 cameraPosition);
		
		// 生成TerrainTiledSplatMapTileRequest
		void ProduceTerrainTiledSplatMapTileRequest(std::vector<TerrainTiledTextureTileRequestTask>& requestTasks, Math::Vector3 cameraPosition, bool useLimit = true);

		// 处理TerrainTiledSplatMapTileRequest
		void ProcessTerrainTiledSplatMapTileRequest(std::vector<TerrainTiledTextureTileRequestTask>& requestTasks);

		// 录制GPU命令
		void RecordGpuCommand(std::vector<TerrainNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand);

		// 加载纹理图集
		void RecordFarTerrainTextureAtlas(TerrainTextureAtlas* terrainTextureAtlas, TerrainNodeRequestTask& requestTask, CommandListWrap& copyCommandList, LinearBufferAllocator* tempLinearBufferAllocator);

		// 设置帧完成回调函数
		void SetupFrameCompletedCallBack();

		// 帧完成回调
		void OnFrameCompleted(uint8_t frameIndex);

		// DSTorage帧完成回调
		void OnDStorageFrameCompleted(uint8_t frameIndex);

	private:
		TerrainRenderer* mRenderer{ nullptr };
		
		// 着色器程序名称
		inline static std::string smUpdateTerrainNodeDescriptorSN = "UpdateTerrainNodeDescriptor";
		inline static uint32_t smThreadSizeInGroup = 8u;
		inline static uint32_t smMaxBackFrameCount = 3u;

		// 线程同步变量
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// 地形数据(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
		std::vector<TerrainLodDescriptor>& mTerrainLodDescriptors;			// 地形全LOD内容描述表
		std::vector<TerrainNodeDescriptor>& mTerrainNodeDescriptors;		// 地形全节点内容描述表
		std::vector<TerrainNodeRuntimeState>& mTerrainNodeRuntimeStates;	// 地形全节点运行时状态

		std::vector<TerrainTiledTextureTileRuntimeState> mTerrainTiledSplatMapTileRuntimeStates;	// Tile全节点运行时状态

		// For TerrainTiledTexutre(SplatMap)
		std::unique_ptr<GHL::CommandQueue>          mBackMappingQueue;
		std::unique_ptr<GHL::Fence>                 mBackMappingFence;
		std::unique_ptr<GHL::DirectStorageQueue>    mBackDStorageQueue;
		std::unique_ptr<GHL::Fence>			        mBackDStorageFence;
		std::unique_ptr<Renderer::RingFrameTracker> mBackDStorageFrameTracker;

		std::unique_ptr<GHL::CommandQueue>                  mBackCopyQueue;
		std::unique_ptr<GHL::Fence>                         mBackCopyFence;
		std::unique_ptr<Renderer::RingFrameTracker>	        mBackCopyFrameTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mBackCopyLinearBufferAllocator;			// 纹理Tile在共享内存中的TempBuffer
		std::unique_ptr<Renderer::PoolCommandListAllocator> mBackCopyCommandListAllocator;

		std::unique_ptr<GHL::CommandQueue>                  mBackComputeQueue;
		std::unique_ptr<GHL::Fence>                         mBackComputeFence;
		std::unique_ptr<Renderer::RingFrameTracker>	        mBackComputeFrameTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mBackComputeLinearBufferAllocator;		// 分配普通的PassData
		std::unique_ptr<Renderer::PoolCommandListAllocator> mBackComputeCommandListAllocator;

		std::unique_ptr<Renderer::ResourceStateTracker>     mBackResourceStateTracker;	

		struct UpdateTerrainNodeDescriptorPassData {
		public:
			uint32_t terrainNodeDescriptorBufferIndex;
			uint32_t updateTerrainNodeDescriptorRequestBufferIndex;
		};
		UpdateTerrainNodeDescriptorPassData mUpdateTerrainNodeDescriptorPassData;
		BufferWrap mUpdateTerrainNodeDescriptorRequestBuffer;						// 用于更新地形节点描述的缓冲

		Tool::ConcurrentQueue<RecordedGpuCommand> mRecordedGpuCommands;				// 该队列由BackThread和MainThread共同访问

		std::vector<std::vector<TerrainNodeRequestTask>> mReservedTerrainNodeRequestTasks;	// 预留的地形节点请求任务，以便帧完成后的回调处理
		std::vector<uint32_t> mFrameCompletedFlags;											// 帧完成标记序列
		inline static uint32_t smFrameCompletedFlag = 2u;									// 帧完成的标记大小

		std::vector<std::vector<TerrainTiledTextureTileRequestTask>> mReservedTerrainTiledTextureTileRequestTasks; // 预留的TiledTextureTile请求任务
	};

}