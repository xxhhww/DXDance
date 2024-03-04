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
		Math::Matrix4 mvpMatrix;		// ת����ͼƬ�ռ��еľ���
	};

	/*
	* ʵʱ���������������
	*/
	struct RuntimeVTNodeRequestTask {
	public:
		RuntimeVTAtlasTileCache::Node* atlasNode{ nullptr };	// һ��ͼ��Ԫ��
		Math::Vector4 runtimeVTRealRect;

		// ǰһ��
		Math::Int2 prevPagePos{ -1, -1 };
		int32_t    prevPageLevel{ -1 };

		// ��һ��
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

			// RuntimeVTRealRectChanged�¼�����ʱ����
			GHL::CommandList*  updateRuntimeVTAtlasInRealRectChangedCommandList{ nullptr };
			GHL::CommandList*  makeRuntimeVTPageTableInvalidCommandList{ nullptr };
			GHL::CommandList*  updateRuntimeVTPageTableInRealRectChangedCommandList{ nullptr };

			uint32_t           frameIndex{ 0u };
		};

	public:
		RuntimeVTBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting);

		~RuntimeVTBackend();

		// Ԥ����
		void Preload();

		void OnFrameLoading(uint32_t frameIndex);

		auto& GetRecordedGpuCommands() { return mRecordedGpuCommands; }
		auto& GetRecordedGpuCommandsInRealRectChanged() { return mRecordedGpuCommandsInRealRectChanged; }

	private:
		// ��̨�߳�
		void BackendThread();

		// ����TerrainFeedback(���ڴ���TerrainFeedbackʱ��ϳ������ڴ���ʱ���ܻᷢ��RuntimeVTRealRectChanged�¼������������Ҫ�ڴ���Feedbackʱһֱ�ж�RuntimeVTRealRectChanged�����Ƿ���)
		bool ProcessTerrainFeedback(std::vector<RuntimeVTNodeRequestTask>& requestTasks, uint32_t completedFenceValue);

		// ����GpuUpdateRuntimeVTAtlasRequests
		void ProduceGpuUpdateRuntimeVTAtlasRequests(std::vector<RuntimeVTNodeRequestTask>& requestTasks, std::vector<GpuUpdateRuntimeVTAtlasRequest>& updateRequests);

		void ProduceGpuUpdateRuntimeVTPageTableRequests(std::vector<RuntimeVTNodeRequestTask>& requestTasks, std::vector<GpuUpdateRuntimeVTPageTableRequest>& updateRequests);

		// ¼��GPU����
		void RecordGpuCommand(std::vector<RuntimeVTNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand);
		
		// ����RuntimeVTRealRectChanged
		void ProcessRuntimeVTRealRectChanged();

		// ¼��GPU����
		void RecordedGpuCommandInRealRectChanged(RecordedGpuCommand& recordedGpuCommand, std::vector<RuntimeVTNodeRequestTask>& requestTasks);

		// ����ͼ�ζ���
		void CreateGraphicsObject();

		// ����֡��ɻص�����
		void SetupFrameCompletedCallBack();

		// ֡��ɻص�
		void OnFrameCompleted(uint8_t frameIndex);

	private:
		TerrainRenderer* mRenderer{ nullptr };

		// ��ɫ����������
		inline static std::string smUpdateRuntimeVTAtlasSN = "UpdateRuntimeVTAtlas";
		inline static std::string smUpdateRuntimeVTPageTableSN = "UpdateRuntimeVTPageTable";
		inline static std::string smRuntimeVTPageTableChangedPass1SN = "RuntimeVTPageTableChangedPass1";
		inline static std::string smRuntimeVTPageTableChangedPass2SN = "RuntimeVTPageTableChangedPass2";
		inline static uint32_t    smThreadSizeInGroup = 8u;

		inline static uint32_t smMaxRvtFrameCount = 3u;

		// �߳�ͬ������
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// ��������(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
		std::vector<RuntimeVTPageTable>& mRvtPageTables;
		Math::Vector4 mRuntimeVTRealRect;	// ������

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
		Renderer::TextureWrap mRuntimeVTPageTableCopy;	// ����

		Tool::ConcurrentQueue<RecordedGpuCommand> mRecordedGpuCommands;							// �ö�����BackThread��MainThread��ͬ����

		std::vector<std::vector<RuntimeVTNodeRequestTask>> mReservedTerrainNodeRequestTasks;	// Ԥ���ĵ��νڵ����������Ա�֡��ɺ�Ļص�����

		struct RuntimeVTPageTableChangedPass1Data {
		public:
			uint32_t runtimeVTPageTableMapIndex;
			uint32_t maxPageLevel;
			uint32_t invalidPageLevelBeg;	// ����һ�ηǷ���PageLevel
			uint32_t invalidPageLevelEnd;

			int32_t  invalidRegionXBeg;		// ����һ�ηǷ�����(u��)
			int32_t  invalidRegionXEnd;
			int32_t  invalidRegionYBeg;		// ����һ�ηǷ�����(v��)
			int32_t  invalidRegionYEnd;
		};

		struct RuntimeVTPageTableChangedPass2Data {
		public:
			uint32_t   srcRuntimeVTPageTableMapIndex;	// source
			uint32_t   dstRuntimeVTPageTableMapIndex;	// target
			Math::Int2 pixelOffset;						// pageƫ��

			int   pageTableMapSize;
			float pad1;
			float pad2;
			float pad3;
		};

		RuntimeVTPageTableChangedPass1Data mRuntimeVTPageTableChangedPass1Data;
		RuntimeVTPageTableChangedPass2Data mRuntimeVTPageTableChangedPass2Data;
		Tool::ConcurrentQueue<RecordedGpuCommand> mRecordedGpuCommandsInRealRectChanged;	// �ö�����BackThread��MainThread��ͬ����
	};

}