#pragma once
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceStateTracker.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"
#include "Renderer/TerrainTextureAtlasTileCache.h"

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

	public:
	};

	/*
	* 描述如何更新地形节点描述的结构体
	*/
	struct GpuUpdateTerrainNodeDescriptorRequest {
	public:
		uint32_t srcTerrainNodeIndex{ 65536u };	// 源地形节点索引(65536为无效值)
		uint32_t dstTerrainNodeIndex;			// 终地形节点索引

		uint32_t tilePosX;	// 终地形节点数据在图集上的坐标(如果srcTerrainNodeIndex存在有效值，则该坐标原先属于srcTerrainNodeIndex，现在被剥夺了)
		uint32_t tilePosY;	// 同上
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
		};

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

		auto& GetRecordedGpuCommands() { return mRecordedGpuCommands; }

	private:
		// 后台线程
		void BackendThread();

		void CreateGraphicsObject();

		// 处理地形节点请求
		void ProcessTerrainNodeRequest(std::vector<TerrainNodeRequestTask>& requestTasks, Math::Vector3 cameraPosition);

		// 录制GPU命令
		void RecordGpuCommand(std::vector<TerrainNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand);

		// 加载纹理图集
		void RecordFarTerrainTextureAtlas(TerrainTextureAtlas* terrainTextureAtlas, TerrainNodeRequestTask& requestTask, CommandListWrap& copyCommandList, LinearBufferAllocator* tempLinearBufferAllocator);

		// 设置帧完成回调函数
		void SetupFrameCompletedCallBack();

		// 帧完成回调
		void OnFrameCompleted(uint8_t frameIndex);

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
		std::vector<TerrainLodDescriptor>& mTerrainLodDescriptors;		// 地形全LOD内容描述表
		std::vector<TerrainNodeDescriptor>& mTerrainNodeDescriptors;		// 地形全节点内容描述表
		std::vector<TerrainNodeRuntimeState>& mTerrainNodeRuntimeStates;	// 地形全节点运行时状态

		std::unique_ptr<GHL::DirectStorageQueue> mBackDStorageQueue;
		std::unique_ptr<GHL::Fence>			     mBackDStorageFence;

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
	};

}