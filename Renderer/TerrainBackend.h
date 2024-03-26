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
	* ������Դ��������
	*/
	struct TerrainNodeRequestTask {
	public:
		TerrainTextureAtlasTileCache::Node* atlasNode{ nullptr };	// һ��ͼ��Ԫ��
		int32_t prevTerrainNodeIndex{ -1 };		// ǰ�ε��νڵ�����
		int32_t nextTerrainNodeIndex{ -1 };		// ���ε��νڵ�����
	};

	/*
	* TerrainTiledRequestTask
	*/
	struct TerrainTiledTextureTileRequestTask {
	public:
		TerrainTiledTextureHeapAllocationCache::Node* cacheNode{ nullptr };	// ����һ��HeapAllocation
		int32_t prevTileIndex{ -1 };	// ǰ��Tile����
		int32_t nextTileIndex{ -1 };	// ����Tile����
	};

	/*
	* ��̨��(���𴴽��������νڵ��ȫ���������������νڵ����ݡ�����GPUפ���ĵ�������������)
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

		// Ԥ����
		void Preload();

		auto& GetRecordedGpuCommands() { return mRecordedGpuCommands; }

		void OnFrameLoading(uint32_t frameIndex);

	private:
		// ��̨�߳�
		void BackendThread();

		void CreateGraphicsObject();

		// ������νڵ�����
		void ProduceTerrainNodeRequest(std::vector<TerrainNodeRequestTask>& requestTasks, Math::Vector3 cameraPosition);
		
		// ����TerrainTiledSplatMapTileRequest
		void ProduceTerrainTiledSplatMapTileRequest(std::vector<TerrainTiledTextureTileRequestTask>& requestTasks, Math::Vector3 cameraPosition, bool useLimit = true);

		// ����TerrainTiledSplatMapTileRequest
		void ProcessTerrainTiledSplatMapTileRequest(std::vector<TerrainTiledTextureTileRequestTask>& requestTasks);

		// ¼��GPU����
		void RecordGpuCommand(std::vector<TerrainNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand);

		// ��������ͼ��
		void RecordFarTerrainTextureAtlas(TerrainTextureAtlas* terrainTextureAtlas, TerrainNodeRequestTask& requestTask, CommandListWrap& copyCommandList, LinearBufferAllocator* tempLinearBufferAllocator);

		// ����֡��ɻص�����
		void SetupFrameCompletedCallBack();

		// ֡��ɻص�
		void OnFrameCompleted(uint8_t frameIndex);

		// DSTorage֡��ɻص�
		void OnDStorageFrameCompleted(uint8_t frameIndex);

	private:
		TerrainRenderer* mRenderer{ nullptr };
		
		// ��ɫ����������
		inline static std::string smUpdateTerrainNodeDescriptorSN = "UpdateTerrainNodeDescriptor";
		inline static uint32_t smThreadSizeInGroup = 8u;
		inline static uint32_t smMaxBackFrameCount = 3u;

		// �߳�ͬ������
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// ��������(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
		std::vector<TerrainLodDescriptor>& mTerrainLodDescriptors;			// ����ȫLOD����������
		std::vector<TerrainNodeDescriptor>& mTerrainNodeDescriptors;		// ����ȫ�ڵ�����������
		std::vector<TerrainNodeRuntimeState>& mTerrainNodeRuntimeStates;	// ����ȫ�ڵ�����ʱ״̬

		std::vector<TerrainTiledTextureTileRuntimeState> mTerrainTiledSplatMapTileRuntimeStates;	// Tileȫ�ڵ�����ʱ״̬

		// For TerrainTiledTexutre(SplatMap)
		std::unique_ptr<GHL::CommandQueue>          mBackMappingQueue;
		std::unique_ptr<GHL::Fence>                 mBackMappingFence;
		std::unique_ptr<GHL::DirectStorageQueue>    mBackDStorageQueue;
		std::unique_ptr<GHL::Fence>			        mBackDStorageFence;
		std::unique_ptr<Renderer::RingFrameTracker> mBackDStorageFrameTracker;

		std::unique_ptr<GHL::CommandQueue>                  mBackCopyQueue;
		std::unique_ptr<GHL::Fence>                         mBackCopyFence;
		std::unique_ptr<Renderer::RingFrameTracker>	        mBackCopyFrameTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mBackCopyLinearBufferAllocator;			// ����Tile�ڹ����ڴ��е�TempBuffer
		std::unique_ptr<Renderer::PoolCommandListAllocator> mBackCopyCommandListAllocator;

		std::unique_ptr<GHL::CommandQueue>                  mBackComputeQueue;
		std::unique_ptr<GHL::Fence>                         mBackComputeFence;
		std::unique_ptr<Renderer::RingFrameTracker>	        mBackComputeFrameTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mBackComputeLinearBufferAllocator;		// ������ͨ��PassData
		std::unique_ptr<Renderer::PoolCommandListAllocator> mBackComputeCommandListAllocator;

		std::unique_ptr<Renderer::ResourceStateTracker>     mBackResourceStateTracker;	

		struct UpdateTerrainNodeDescriptorPassData {
		public:
			uint32_t terrainNodeDescriptorBufferIndex;
			uint32_t updateTerrainNodeDescriptorRequestBufferIndex;
		};
		UpdateTerrainNodeDescriptorPassData mUpdateTerrainNodeDescriptorPassData;
		BufferWrap mUpdateTerrainNodeDescriptorRequestBuffer;						// ���ڸ��µ��νڵ������Ļ���

		Tool::ConcurrentQueue<RecordedGpuCommand> mRecordedGpuCommands;				// �ö�����BackThread��MainThread��ͬ����

		std::vector<std::vector<TerrainNodeRequestTask>> mReservedTerrainNodeRequestTasks;	// Ԥ���ĵ��νڵ����������Ա�֡��ɺ�Ļص�����
		std::vector<uint32_t> mFrameCompletedFlags;											// ֡��ɱ������
		inline static uint32_t smFrameCompletedFlag = 2u;									// ֡��ɵı�Ǵ�С

		std::vector<std::vector<TerrainTiledTextureTileRequestTask>> mReservedTerrainTiledTextureTileRequestTasks; // Ԥ����TiledTextureTile��������
	};

}