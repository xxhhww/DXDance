#pragma once
#include "Renderer/TerrainSetting.h"
#include "GHL/Fence.h"

#include <memory>
#include <vector>
#include <thread>
#include <DirectStorage/dstorage.h>

namespace Renderer {

	class RenderEngine;
	class BuddyHeapAllocator;
	class PoolDescriptorAllocator;
	
	class TerrainTextureAtlas;
	class TerrainTiledTexture;
	class TerrainTextureArray;
	class TerrainBackend;
	class TerrainPipelinePass;

	struct TerrainNodeID {
	public:
		uint8_t currLod;
		uint8_t posX;
		uint8_t posY;
	};

	struct TerrainLodDescriptor {
	public:
		uint32_t nodeMeterSize;		// ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
		uint32_t nodeStartOffset;	// ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
		uint32_t nodeCount;			// ��LOD�е�Node���ܸ���

		float pad1;
	};

	struct TerrainNodeDescriptor {
	public:
		uint16_t minHeight{ 0u };		// HeightMapΪR16��ʽ
		uint16_t maxHeight{ 0u };		// HeightMapΪR16��ʽ

		uint16_t heightMapIndex{ 0u };
		uint16_t pad;

		uint16_t colorMapIndex{ 0u };
		uint16_t normalMapIndex{ 0u };
	};

	enum class ResourceResidencyState : uint8_t {
		NotResident = 0,
		Resident = 1,
		Loading = 2,
	};

	struct TerrainNodeRuntimeState {
	public:
		ResourceResidencyState residencyState{ ResourceResidencyState::NotResident };
	};

	class TerrainRenderer {
	public:
		TerrainRenderer(RenderEngine* renderEngine);
		~TerrainRenderer();

		void Initialize();

		void AddPass();

		void Update();

		inline auto* GetRenderEngine() const { return mRenderEngine; }
		inline auto* GetHeapAllocator() const { return mHeapAllocator; }
		inline auto* GetDescriptorAllocator() const { return mDescriptorAllocator; }

		inline auto* GetDStorageFactory() const { return mDstorageFactory; }
		inline auto* GetDStorageFileQueue() const { return mDStorageFileQueue; }
		inline auto* GetCopyFence() const { return mCopyFence; }


		inline auto* GetFarTerrainHeightMapAtlas() const { return mFarTerrainHeightMapAtlas.get(); }
		inline auto* GetFarTerrainAlbedoMapAtlas() const { return mFarTerrainAlbedoMapAtlas.get(); }
		inline auto* GetFarTerrainNormalMapAtlas() const { return mFarTerrainNormalMapAtlas.get(); }
		
		inline auto* GetNearTerrainTiledHeightMap() const { return mNearTerrainTiledHeightMap.get(); }
		inline auto* GetNearTerrainTiledNormalMap() const { return mNearTerrainTiledNormalMap.get(); }
		inline auto* GetNearTerrainTiledSplatMap()  const { return mNearTerrainTiledSplatMap.get(); }

		inline auto* GetNearTerrainAlbedoArray() const { return mNearTerrainAlbedoArray.get(); }
		inline auto* GetNearTerrainNormalArray() const { return mNearTerrainNormalArray.get(); }

	private:
		RenderEngine* mRenderEngine{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		IDStorageFactory* mDstorageFactory{ nullptr };
		IDStorageQueue* mDStorageFileQueue{ nullptr };
		GHL::Fence* mCopyFence{ nullptr };

		TerrainSetting mTerrainSetting;

		std::vector<TerrainLodDescriptor>  mTerrainLodDescriptors;		// ����ȫLOD����������
		std::vector<TerrainNodeDescriptor> mTerrainNodeDescriptors;		// ����ȫ�ڵ�����������
		std::vector<TerrainNodeRuntimeState> mTerrainNodeRuntimeStates;	// ���νڵ�ʵʱ״̬��

		HANDLE mHasPreloaded{ nullptr };
		std::unique_ptr<TerrainBackend> mTerrainBackend;

		std::unique_ptr<TerrainPipelinePass> mTerrainPipelinePass;

		// ������Զ���������Ⱦ������ͼ����ͼ����ÿ��Ԫ�ض���65 * 65��С��Tile
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainHeightMapAtlas;
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainAlbedoMapAtlas;
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainNormalMapAtlas;

		// �ʺ��ڽ�������Ⱦ������ͼ��������ԭ������������ͼ����ͬ��ͼ����Tile�Ĵ�Сȡ����ͼƬ�����ݸ�ʽ
		std::unique_ptr<TerrainTiledTexture> mNearTerrainTiledHeightMap;
		std::unique_ptr<TerrainTiledTexture> mNearTerrainTiledSplatMap;
		std::unique_ptr<TerrainTiledTexture> mNearTerrainTiledNormalMap;

		// ��������λ����������
		std::unique_ptr<TerrainTextureArray> mNearTerrainAlbedoArray;
		std::unique_ptr<TerrainTextureArray> mNearTerrainNormalArray;
	};

}