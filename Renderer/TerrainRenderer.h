#pragma once
#include "Renderer/TerrainSetting.h"
#include "Renderer/ResourceAllocator.h"

#include "Renderer/TerrainTextureAtlasTileCache.h"

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
	class TerrainBackend;
	class TerrainPipelinePass;

	struct TerrainLodDescriptor {
	public:
		uint32_t nodeMeterSize;		// ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
		uint32_t nodeStartOffset;	// ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
		uint32_t nodeCount;			// ��LOD�е�Node���ܸ���

		float pad1;
	};

	struct TerrainNodeRuntimeState {
	public:
		bool inQueue{ false };		// �Ƿ�λ�����������
		bool inLoading{ false };	// ��GPU������
		bool inTexture{ false };	// ������ͼ����

		TerrainTextureAtlasTileCache::Node* atlasNode{ nullptr };

	public:
		inline void SetInActive()  { inQueue = false; inLoading = false; inTexture = false; }
		inline void SetInQueue()   { inQueue = true;  inLoading = false; inTexture = false; }
		inline void SetInLoading() { inQueue = false; inLoading = true;  inTexture = false; }
		inline void SetInTexture() { inQueue = false; inLoading = false; inTexture = true;  }
	};

	struct TerrainNodeDescriptor {
	public:
		uint32_t minHeight{ 0u };
		uint32_t maxHeight{ 0u };

		uint32_t tilePosX{ 0u };			// 255��ʾ��Դδ����
		uint32_t tilePosY{ 0u };			// 255��ʾ��Դδ����
	};

	class TerrainRenderer {
		friend class TerrainBackend;
		friend class TerrainTextureAtlas;
		friend class TerrainTextureArray;
	public:
		TerrainRenderer(RenderEngine* renderEngine);
		~TerrainRenderer();

		void Initialize();

		void AddPass();

		void Update();

		inline auto* GetRenderEngine() const { return mRenderEngine; }

		inline auto* GetFarTerrainHeightMapAtlas() const { return mFarTerrainHeightMapAtlas.get(); }
		inline auto* GetFarTerrainAlbedoMapAtlas() const { return mFarTerrainAlbedoMapAtlas.get(); }
		inline auto* GetFarTerrainNormalMapAtlas() const { return mFarTerrainNormalMapAtlas.get(); }
		
		inline auto* GetNearTerrainAlbedoArray() const { return mNearTerrainAlbedoArray.get(); }
		inline auto* GetNearTerrainNormalArray() const { return mNearTerrainNormalArray.get(); }

	private:
		RenderEngine* mRenderEngine{ nullptr };

		TerrainSetting mTerrainSetting;

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

		std::unique_ptr<TerrainBackend> mTerrainBackend;

		std::unique_ptr<TerrainPipelinePass> mTerrainPipelinePass;

		Renderer::BufferWrap mTerrainLodDescriptorBuffer;	// GPU����ȫLOD״̬��ֻ������		
		Renderer::BufferWrap mTerrainNodeDescriptorBuffer;	// GPU����ȫ�ڵ�״̬��������Ⱦ�߳����̨�߳�(ÿ��Ӧ��ֻ���²��ֽڵ�)���ʣ��ö���������Rvt�е�PageTable
	};

}