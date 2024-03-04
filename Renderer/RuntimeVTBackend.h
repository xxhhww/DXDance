#pragma once
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceStateTracker.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"
#include "Renderer/RuntimeVTAtlasTileCache.h"
#include "Renderer/RuntimeVTPageTable.h"

#include "Tools/ConcurrentQueue.h"

namespace Renderer {

	struct GpuUpdateRuntimeVTAtlasRequest {
	public:
		Math::Matrix4 mvpMatrix;
		Math::Vector4 tileOffset;
		Math::Vector4 blendOffset;
	};

	struct GpuUpdateRuntimeVTPageTableRequest {
	public:
		uint32_t tilePosX;
		uint32_t tilePosY;
		int32_t  pageLevel;
		float    pad1;

		Math::Int4 rectInPage0Level;
		Math::Matrix4 mvpMatrix;		// 转换到图片空间中的矩阵
	};

	/*
	* 实时虚拟纹理调度请求
	*/
	struct RuntimeVTNodeRequestTask {
	public:
		RuntimeVTAtlasTileCache::Node* atlasNode{ nullptr };	// 一个图集元素
		Math::Vector4 runtimeVTRealRect;

		// 前一任
		Math::Int2 prevPagePos{ -1, -1 };
		int32_t    prevPageLevel{ -1 };

		// 下一任
		Math::Int2 nextPagePos{ -1, -1 };
		int32_t    nextPageLevel{ -1 };
	};

	class RuntimeVTBackend {
	public:
		struct RecordedGpuCommand {
		public:
			GHL::CommandQueue* graphicsQueue{ nullptr };
			GHL::Fence*        graphicsFence{ nullptr };
			uint64_t           graphicsFenceExpectedValue{ 0u };

			GHL::CommandList*  updateRuntimeVTAtlasCommandList{ nullptr };
			GHL::CommandList*  updateRuntimeVTPageTableCommandList{ nullptr };

			// RuntimeVTRealRectChanged事件发生时访问
			GHL::CommandList*  updateRuntimeVTAtlasInRealRectChangedCommandList{ nullptr };
			GHL::CommandList*  makeRuntimeVTPageTableInvalidCommandList{ nullptr };
			GHL::CommandList*  updateRuntimeVTPageTableInRealRectChangedCommandList{ nullptr };

			uint32_t           frameIndex{ 0u };
		};

	public:
		RuntimeVTBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting);

		~RuntimeVTBackend();

		// 预加载
		void Preload();

		void OnFrameLoading(uint32_t frameIndex);

		auto& GetRecordedGpuCommands() { return mRecordedGpuCommands; }
		auto& GetRecordedGpuCommandsInRealRectChanged() { return mRecordedGpuCommandsInRealRectChanged; }

	private:
		// 后台线程
		void BackendThread();

		// 处理TerrainFeedback(由于处理TerrainFeedback时间较长，且在处理时可能会发生RuntimeVTRealRectChanged事件，因此我们需要在处理Feedback时一直判断RuntimeVTRealRectChanged事情是否发生)
		bool ProcessTerrainFeedback(std::vector<RuntimeVTNodeRequestTask>& requestTasks, uint32_t completedFenceValue);

		// 生成GpuUpdateRuntimeVTAtlasRequests
		void ProduceGpuUpdateRuntimeVTAtlasRequests(std::vector<RuntimeVTNodeRequestTask>& requestTasks, std::vector<GpuUpdateRuntimeVTAtlasRequest>& updateRequests);

		void ProduceGpuUpdateRuntimeVTPageTableRequests(std::vector<RuntimeVTNodeRequestTask>& requestTasks, std::vector<GpuUpdateRuntimeVTPageTableRequest>& updateRequests);

		// 录制GPU命令
		void RecordGpuCommand(std::vector<RuntimeVTNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand);
		
		// 处理RuntimeVTRealRectChanged
		void ProcessRuntimeVTRealRectChanged();

		// 录制GPU命令
		void RecordedGpuCommandInRealRectChanged(RecordedGpuCommand& recordedGpuCommand, std::vector<RuntimeVTNodeRequestTask>& requestTasks);

		// 创建图形对象
		void CreateGraphicsObject();

		// 设置帧完成回调函数
		void SetupFrameCompletedCallBack();

		// 帧完成回调
		void OnFrameCompleted(uint8_t frameIndex);

	private:
		TerrainRenderer* mRenderer{ nullptr };

		// 着色器程序名称
		inline static std::string smUpdateRuntimeVTAtlasSN = "UpdateRuntimeVTAtlas";
		inline static std::string smUpdateRuntimeVTPageTableSN = "UpdateRuntimeVTPageTable";
		inline static std::string smRuntimeVTPageTableChangedPass1SN = "RuntimeVTPageTableChangedPass1";
		inline static std::string smRuntimeVTPageTableChangedPass2SN = "RuntimeVTPageTableChangedPass2";
		inline static uint32_t    smThreadSizeInGroup = 8u;

		inline static uint32_t smMaxRvtFrameCount = 3u;

		// 线程同步变量
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// 地形数据(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
		std::vector<RuntimeVTPageTable>& mRvtPageTables;
		Math::Vector4 mRuntimeVTRealRect;	// 非引用

		std::unique_ptr<Renderer::BuddyHeapAllocator> mTerrainTiledSplatMapHeapAllocator;


		std::unique_ptr<GHL::CommandQueue>                  mRvtGraphicsQueue;
		std::unique_ptr<GHL::Fence>                         mRvtGraphicsFence;

		std::unique_ptr<GHL::CommandQueue>                  mRvtComputeQueue;
		std::unique_ptr<GHL::Fence>                         mRvtComputeFence;
		
		std::unique_ptr<Renderer::RingFrameTracker>	        mRvtFrameTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mRvtLinearBufferAllocator;
		std::unique_ptr<Renderer::PoolCommandListAllocator> mRvtCommandListAllocator;

		std::unique_ptr<Renderer::ResourceStateTracker>     mRvtResourceStateTracker;

		uint32_t   mQuadMeshVertexCountPerAxis;
		BufferWrap mQuadMeshVertexBuffer;
		BufferWrap mQuadMeshIndexBuffer;
		uint32_t   mQuadMeshIndexCount;

		struct UpdateRuntimeVTAtlasPassData {
		public:
			uint32_t drawRequestBufferIndex;
			uint32_t terrainSplatMapIndex;
			uint32_t terrainAlbedoTextureArrayIndex;
			uint32_t terrainNormalTextureArrayIndex;

			uint32_t terrainRoughnessTextureArrayIndex;
			float    pad1;
			float    pad2;
			float    pad3;
		};
		UpdateRuntimeVTAtlasPassData mUpdateRuntimeVTAtlasPassData;
		BufferWrap mUpdateRuntimeVTAtlasRequestBuffer;

		struct UpdateRuntimeVTPageTablePassData {
		public:
			uint32_t drawRequestBufferIndex;
			uint32_t runtimeVTPageTableCopyIndex;
			uint32_t preLoad;
			float pad3;
		};
		UpdateRuntimeVTPageTablePassData mUpdateRuntimeVTPageTablePassData;
		BufferWrap mUpdateRuntimeVTPageTableRequestBuffer;
		Renderer::TextureWrap mRuntimeVTPageTableCopy;	// 副本

		Tool::ConcurrentQueue<RecordedGpuCommand> mRecordedGpuCommands;							// 该队列由BackThread和MainThread共同访问

		std::vector<std::vector<RuntimeVTNodeRequestTask>> mReservedTerrainNodeRequestTasks;	// 预留的地形节点请求任务，以便帧完成后的回调处理

		struct RuntimeVTPageTableChangedPass1Data {
		public:
			uint32_t runtimeVTPageTableMapIndex;
			uint32_t maxPageLevel;
			uint32_t invalidPageLevelBeg;	// 描述一段非法的PageLevel
			uint32_t invalidPageLevelEnd;

			int32_t  invalidRegionXBeg;		// 描述一段非法区域(u轴)
			int32_t  invalidRegionXEnd;
			int32_t  invalidRegionYBeg;		// 描述一段非法区域(v轴)
			int32_t  invalidRegionYEnd;
		};

		struct RuntimeVTPageTableChangedPass2Data {
		public:
			uint32_t   srcRuntimeVTPageTableMapIndex;	// source
			uint32_t   dstRuntimeVTPageTableMapIndex;	// target
			Math::Int2 pixelOffset;						// page偏移

			int   pageTableMapSize;
			float pad1;
			float pad2;
			float pad3;
		};

		RuntimeVTPageTableChangedPass1Data mRuntimeVTPageTableChangedPass1Data;
		RuntimeVTPageTableChangedPass2Data mRuntimeVTPageTableChangedPass2Data;
		Tool::ConcurrentQueue<RecordedGpuCommand> mRecordedGpuCommandsInRealRectChanged;	// 该队列由BackThread和MainThread共同访问
	};

}