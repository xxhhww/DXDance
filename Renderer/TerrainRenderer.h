#pragma once
#include "Renderer/TerrainSetting.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/TerrainTextureAtlasTileCache.h"
#include "Renderer/TerrainTiledTextureHeapAllocationCache.h"
#include "Renderer/RuntimeVTPageTable.h"
#include "Renderer/RuntimeVTAtlasTileCache.h"

#include "GHL/Fence.h"
#include "GHL/CommandQueue.h"

#include "Tools/Pool.h"

#include <memory>
#include <vector>
#include <thread>
#include <DirectStorage/dstorage.h>

namespace Renderer {

	class RenderEngine;
	class BuddyHeapAllocator;
	class ResourceStateTracker;
	class PoolDescriptorAllocator;
	
	class TerrainTextureAtlas;
	class TerrainTextureArray;
	class TerrainTiledTexture;
	class TerrainBackend;
	class TerrainPipelinePass;

	class GrasslandLinearBuffer;

	class RuntimeVTBackend;
	class RuntimeVTAtlas;

	struct TerrainLodDescriptor {
	public:
		uint32_t nodeMeterSize;		// ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
		uint32_t nodeStartOffset;	// ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
		uint32_t nodeCount;			// ��LOD�е�Node���ܸ���

		float pad1;
	};

	struct TerrainNodeRuntimeState {
	public:

		bool inReady{ false };		// �Ƿ���CPU�˷��䵽һ��AtlasNode
		bool inQueue{ false };		// �Ƿ�λ��GPU�����ύ������
		bool inLoading{ false };	// �Ƿ�GPU�����ѱ��ύ

		bool inReadyOut{ false };
		bool inQueueOut{ false };
		bool inLoadingOut{ false };

		bool inTexture{ false };	// �Ƿ���������ͼ����

		TerrainTextureAtlasTileCache::Node* atlasNode{ nullptr };

	public:
		inline void SetInReady()   { inReady = true;  inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = false; inTexture = false; }
		inline void SetInQueue()   { inReady = false; inQueue = true;  inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = false; inTexture = false; }
		inline void SetInLoading() { inReady = false; inQueue = false; inLoading = true;  inReadyOut = false; inQueueOut = false; inLoadingOut = false; inTexture = false; }

		inline void SetInReadyOut()   { inReady = false; inQueue = false; inLoading = false; inReadyOut = true;  inQueueOut = false; inLoadingOut = false; inTexture = true; }
		inline void SetInQueueOut()   { inReady = false; inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = true;  inLoadingOut = false; inTexture = true; }
		inline void SetInLoadingOut() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = true;  inTexture = true; }

		inline void SetInTexture()  { inReady = false; inQueue = false; inLoading = false; inReadyOut = false;  inQueueOut = false; inLoadingOut = false; inTexture = true;  }
		inline void SetOutTexture() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false;  inQueueOut = false; inLoadingOut = false; inTexture = false; }
	};

	struct TerrainNodeDescriptor {
	public:
		float minHeight;
		float maxHeight;

		uint32_t tilePosX;	// 255��ʾ��Դδ����
		uint32_t tilePosY;	// 255��ʾ��Դδ����
	};

	struct TerrainTiledTextureTileRuntimeState {
	public:
		bool inReady{ false };		// �Ƿ���CPU�˷��䵽һ��AtlasNode
		bool inQueue{ false };		// �Ƿ�λ��GPU�����ύ������
		bool inLoading{ false };	// �Ƿ�GPU�����ѱ��ύ

		bool inReadyOut{ false };
		bool inQueueOut{ false };
		bool inLoadingOut{ false };

		bool inTexture{ false };	// �Ƿ���������ͼ����

		TerrainTiledTextureHeapAllocationCache::Node* cacheNode{ nullptr };

	public:
		inline void SetInReady() { inReady = true;  inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = false; inTexture = false; }
		inline void SetInQueue() { inReady = false; inQueue = true;  inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = false; inTexture = false; }
		inline void SetInLoading() { inReady = false; inQueue = false; inLoading = true;  inReadyOut = false; inQueueOut = false; inLoadingOut = false; inTexture = false; }

		inline void SetInReadyOut() { inReady = false; inQueue = false; inLoading = false; inReadyOut = true;  inQueueOut = false; inLoadingOut = false; inTexture = true; }
		inline void SetInQueueOut() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = true;  inLoadingOut = false; inTexture = true; }
		inline void SetInLoadingOut() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false; inQueueOut = false; inLoadingOut = true;  inTexture = true; }

		inline void SetInTexture() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false;  inQueueOut = false; inLoadingOut = false; inTexture = true; }
		inline void SetOutTexture() { inReady = false; inQueue = false; inLoading = false; inReadyOut = false;  inQueueOut = false; inLoadingOut = false; inTexture = false; }
	};

	class TerrainRenderer {
		friend class TerrainBackend;
		friend class TerrainTextureAtlas;
		friend class TerrainTextureArray;
		friend class TerrainTiledTexture;
		friend class TerrainPipelinePass;

		friend class GrasslandLinearBuffer;

		friend class RuntimeVTBackend;
		friend class RuntimeVTAtlas;

	public:
		TerrainRenderer(RenderEngine* renderEngine);
		~TerrainRenderer();

		void Initialize();

		void AddPass();

		void Update();

		inline auto* GetRenderEngine() const { return mRenderEngine; }

		inline auto& GetTerrainLodDescriptors()    { return mTerrainLodDescriptors; }
		inline auto& GetTerrainNodeDescriptors()   { return mTerrainNodeDescriptors; }
		inline auto& GetTerrainNodeRuntimeStates() { return mTerrainNodeRuntimeStates; }

		inline const auto& GetTerrainLodDescriptors()    const { return mTerrainLodDescriptors; }
		inline const auto& GetTerrainNodeDescriptors()   const { return mTerrainNodeDescriptors; }
		inline const auto& GetTerrainNodeRuntimeStates() const { return mTerrainNodeRuntimeStates; }

		inline auto* GetFarTerrainHeightMapAtlas() const { return mFarTerrainHeightMapAtlas.get(); }
		inline auto* GetFarTerrainAlbedoMapAtlas() const { return mFarTerrainAlbedoMapAtlas.get(); }
		inline auto* GetFarTerrainNormalMapAtlas() const { return mFarTerrainNormalMapAtlas.get(); }
		
		inline auto* GetNearTerrainAlbedoArray() const { return mNearTerrainAlbedoArray.get(); }
		inline auto* GetNearTerrainNormalArray() const { return mNearTerrainNormalArray.get(); }

		inline auto& GetRuntimeVTPageTables() { return mRuntimeVTPageTables; }
		inline const auto& GetRuntimeVTPageTables() const { return mRuntimeVTPageTables; }
		
		inline auto& GetRuntimeVTPageTableMap() { return mRuntimeVTPageTableMap; }

		inline auto* GetRuntimeVTAlbedoAtlas()    const { return mRuntimeVTAlbedoAtlas.get(); }
		inline auto* GetRuntimeVTNormalAtlas()    const { return mRuntimeVTNormalAtlas.get(); }
		inline auto* GetRuntimeVTAtlasTileCache() const { return mRuntimeVTAtlasTileCache.get(); }

		inline auto* GetTerrainTiledSplatMap() const { return mTerrainTiledSplatMap.get(); }

		inline const auto& GetMaxPageLevel()  const { return mMaxPageLevel; }
		inline const auto& GetPageLevelBias() const { return mPageLevelBias; }

		inline const auto& GetRuntimeVTRealRect() const { return mRuntimeVTRealRect; }
		inline const auto& GetRuntimeVTRealRectOffsetInPage0Level() const { return mRuntimeVTRealRectOffsetInPage0Level; }

		void OnRuntimeVTRealRectChanged(const Math::Vector4& currRuntimeVTRealRect);

		bool CheckRuntimeVTRealRectChanged();
		void SetRuntimeVTRealRectChangedCompletedEvnet();

	private:
		RenderEngine* mRenderEngine{ nullptr };

		TerrainSetting mTerrainSetting;

		std::unique_ptr<GHL::CommandQueue> mTempMappingQueue;
		std::unique_ptr<GHL::Fence>        mTempMappingFence;

		std::vector<TerrainLodDescriptor>    mTerrainLodDescriptors;		// ����ȫLOD����������
		std::vector<TerrainNodeDescriptor>   mTerrainNodeDescriptors;		// ����ȫ�ڵ�����������
		std::vector<TerrainNodeRuntimeState> mTerrainNodeRuntimeStates;		// ����ȫ�ڵ�����ʱ״̬

		// ������Զ���������Ⱦ������ͼ����ͼ����ÿ��Ԫ�ض���65 * 65��С��Tile
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainHeightMapAtlas;
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainAlbedoMapAtlas;
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainNormalMapAtlas;

		std::unique_ptr<TerrainTextureAtlasTileCache> mFarTerrainTextureAtlasTileCache;

		// ��������λ����������
		std::unique_ptr<TerrainTextureArray> mNearTerrainAlbedoArray;
		std::unique_ptr<TerrainTextureArray> mNearTerrainNormalArray;
		std::unique_ptr<BuddyHeapAllocator> mTerrainTextureArrayHeapAllocator;

		std::unique_ptr<TerrainBackend> mTerrainBackend;

		std::unique_ptr<TerrainPipelinePass> mTerrainPipelinePass;

		Renderer::BufferWrap mTerrainLodDescriptorBuffer;	// GPU����ȫLOD״̬��ֻ������		
		Renderer::BufferWrap mTerrainNodeDescriptorBuffer;	// GPU����ȫ�ڵ�״̬��������Ⱦ�߳����̨�߳�(ÿ��Ӧ��ֻ���²��ֽڵ�)���ʣ��ö���������Rvt�е�PageTable

		std::unique_ptr<TerrainTiledTexture> mTerrainTiledSplatMap;											// SplatMap
		std::unique_ptr<BuddyHeapAllocator>  mTerrainTiledSplatMapHeapAllocator;							// HeapAllocator
		std::vector<TerrainTiledTextureTileRuntimeState> mTerrainTiledSplatMapTileRuntimeStates;			// ȫTile����ʱ״̬
		std::unique_ptr<TerrainTiledTextureHeapAllocationCache> mTerrainTiledSplatMapHeapAllocationCache;	// HeapAllocatonCache
		Microsoft::WRL::ComPtr<ID3D12Resource> mTerrainTiledSplatMapBackend;
		D3D12_SUBRESOURCE_TILING mTerrainTiledSplatMapBackendTiling;

		std::unique_ptr<TerrainTiledTexture> mTerrainTiledGrassLandMap;											// SplatMap
		std::unique_ptr<BuddyHeapAllocator>  mTerrainTiledGrassLandMapHeapAllocator;							// HeapAllocator
		std::vector<TerrainTiledTextureTileRuntimeState> mTerrainTiledGrassLandMapTileRuntimeStates;			// ȫTile����ʱ״̬
		std::unique_ptr<TerrainTiledTextureHeapAllocationCache> mTerrainTiledGrassLandMapHeapAllocationCache;	// HeapAllocatonCache
		Microsoft::WRL::ComPtr<ID3D12Resource> mTerrainTiledGrassLandMapBackend;
		D3D12_SUBRESOURCE_TILING mTerrainTiledGrassLandMapBackendTiling;

		// ʵʱ���������̨
		std::unique_ptr<RuntimeVTBackend> mRuntimeVTBackend;

		// TerrainFeedback & TerrainFeedbackReadback
		Renderer::TextureWrap mTerrainFeedbackMap;
		Renderer::TextureWrap mTerrainFeedbackDepthMap;
		struct FeedbackReadbackQueued {
		public:
			uint64_t renderFrameFenceValue{ 0u };	// �ñ�������Ⱦ���߳�д�� RuntimeVTBackend�߳�ֻ��
			Math::Vector4 runtimeVTRealRect;		// �ñ�������Ⱦ���߳�д�� RuntimeVTBackend�߳�ֻ��
			std::atomic<bool> isFresh{ false };		// �ñ�������Ⱦ���߳����� RuntimeVTBackend�߳̽��з������޸�
		};
		std::vector<FeedbackReadbackQueued> mQueuedFeedbackReadbacks;
		std::vector<Renderer::BufferWrap>   mTerrainFeedbackReadbackBuffers;

		// RuntimeVTPageTable
		uint32_t mMaxPageLevel;
		uint32_t mPageLevelBias;
		Math::Vector4 mRuntimeVTRealRect;
		Math::Int2 mRuntimeVTRealRectOffsetInPage0Level;
		HANDLE mRuntimeVTRealRectChangedCompletedEvent{ NULL };
		std::atomic<uint32_t> mRuntimeVTRealRectChangedFlag{ 0u };
		std::vector<RuntimeVTPageTable> mRuntimeVTPageTables;

		Renderer::TextureWrap mRuntimeVTPageTableMap;

		// �����ڽ�������Ⱦ��ʵʱ��������ͼ��
		std::unique_ptr<RuntimeVTAtlas> mRuntimeVTAlbedoAtlas;
		std::unique_ptr<RuntimeVTAtlas> mRuntimeVTNormalAtlas;
		std::unique_ptr<RuntimeVTAtlasTileCache> mRuntimeVTAtlasTileCache;
	};

}