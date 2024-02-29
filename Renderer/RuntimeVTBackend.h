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

	/*
	* ʵʱ���������������
	*/
	struct RuntimeVTNodeRequestTask {
	public:
		RuntimeVTAtlasTileCache::Node* atlasNode{ nullptr };	// һ��ͼ��Ԫ��

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

			GHL::CommandList*  updateRuntimeVTTextureAtlasCommandList{ nullptr };
			GHL::CommandList*  updateRuntimeVTPageTableCommandList{ nullptr };

			uint32_t           frameIndex{ 0u };
		};

	public:
		RuntimeVTBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting);

		~RuntimeVTBackend();

		// Ԥ����
		void Preload();

		void OnFrameLoading(uint32_t frameIndex);

		auto& GetRecordedGpuCommands() { return mRecordedGpuCommands; }

	private:
		// ��̨�߳�
		void BackendThread();

		// ����Feedback
		void ProcessTerrainFeedback(std::vector<RuntimeVTNodeRequestTask>& requestTasks, uint32_t completedFenceValue);

		// ¼��GPU����
		void RecordGpuCommand(std::vector<RuntimeVTNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand);

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
		inline static uint32_t smMaxRvtFrameCount = 3u;

		// �߳�ͬ������
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// ��������(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
		std::vector<RuntimeVTPageTable>& mRvtPageTables;

		std::unique_ptr<Renderer::BuddyHeapAllocator> mTerrainTiledSplatMapHeapAllocator;


		std::unique_ptr<GHL::CommandQueue>                  mRvtGraphicsQueue;
		std::unique_ptr<GHL::Fence>                         mRvtGraphicsFence;
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

		struct UpdateLookupPageTablePassData {
		public:

		};
		UpdateLookupPageTablePassData mUpdateLookupPageTablePassData;
		BufferWrap mUpdateLookupPageTableRequestBuffer;

		Tool::ConcurrentQueue<RecordedGpuCommand> mRecordedGpuCommands;										// �ö�����BackThread��MainThread��ͬ����

		std::vector<std::vector<RuntimeVTNodeRequestTask>> mReservedTerrainNodeRequestTasks;	// Ԥ���ĵ��νڵ����������Ա�֡��ɺ�Ļص�����

	};

}