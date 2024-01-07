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
		uint32_t nodeMeterSize;		// 该LOD中每一个Node的边长(米)(Node是正方形)
		uint32_t nodeStartOffset;	// 该LOD中的第一个Node的开始偏移量
		uint32_t nodeCount;			// 该LOD中的Node的总个数

		float pad1;
	};

	struct TerrainNodeDescriptor {
	public:
		uint16_t minHeight{ 0u };		// HeightMap为R16格式
		uint16_t maxHeight{ 0u };		// HeightMap为R16格式

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

		std::vector<TerrainLodDescriptor>  mTerrainLodDescriptors;		// 地形全LOD内容描述表
		std::vector<TerrainNodeDescriptor> mTerrainNodeDescriptors;		// 地形全节点内容描述表
		std::vector<TerrainNodeRuntimeState> mTerrainNodeRuntimeStates;	// 地形节点实时状态表

		HANDLE mHasPreloaded{ nullptr };
		std::unique_ptr<TerrainBackend> mTerrainBackend;

		std::unique_ptr<TerrainPipelinePass> mTerrainPipelinePass;

		// 适用于远距离地形渲染的纹理图集，图集中每个元素都是65 * 65大小的Tile
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainHeightMapAtlas;
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainAlbedoMapAtlas;
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainNormalMapAtlas;

		// 适合于近距离渲染的纹理图集，调度原理与上述纹理图集不同，图集中Tile的大小取决于图片的数据格式
		std::unique_ptr<TerrainTiledTexture> mNearTerrainTiledHeightMap;
		std::unique_ptr<TerrainTiledTexture> mNearTerrainTiledSplatMap;
		std::unique_ptr<TerrainTiledTexture> mNearTerrainTiledNormalMap;

		// 近距离地形混合所用纹理
		std::unique_ptr<TerrainTextureArray> mNearTerrainAlbedoArray;
		std::unique_ptr<TerrainTextureArray> mNearTerrainNormalArray;
	};

}