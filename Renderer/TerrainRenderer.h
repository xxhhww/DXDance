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
		uint32_t nodeMeterSize;		// 该LOD中每一个Node的边长(米)(Node是正方形)
		uint32_t nodeStartOffset;	// 该LOD中的第一个Node的开始偏移量
		uint32_t nodeCount;			// 该LOD中的Node的总个数

		float pad1;
	};

	struct TerrainNodeRuntimeState {
	public:
		bool inQueue{ false };		// 是否位于任务队列中
		bool inLoading{ false };	// 在GPU加载中
		bool inTexture{ false };	// 在纹理图集中

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

		uint32_t tilePosX{ 0u };			// 255表示资源未加载
		uint32_t tilePosY{ 0u };			// 255表示资源未加载
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

		std::vector<TerrainLodDescriptor>    mTerrainLodDescriptors;		// 地形全LOD内容描述表
		std::vector<TerrainNodeDescriptor>   mTerrainNodeDescriptors;		// 地形全节点内容描述表
		std::vector<TerrainNodeRuntimeState> mTerrainNodeRuntimeStates;		// 地形全节点运行时状态

		// 适用于远距离地形渲染的纹理图集，图集中每个元素都是65 * 65大小的Tile
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainHeightMapAtlas;
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainAlbedoMapAtlas;
		std::unique_ptr<TerrainTextureAtlas> mFarTerrainNormalMapAtlas;

		std::unique_ptr<TerrainTextureAtlasTileCache> mFarTerrainTextureAtlasTileCache;

		// 近距离地形混合所用纹理
		std::unique_ptr<TerrainTextureArray> mNearTerrainAlbedoArray;
		std::unique_ptr<TerrainTextureArray> mNearTerrainNormalArray;

		std::unique_ptr<TerrainBackend> mTerrainBackend;

		std::unique_ptr<TerrainPipelinePass> mTerrainPipelinePass;

		Renderer::BufferWrap mTerrainLodDescriptorBuffer;	// GPU地形全LOD状态表，只被访问		
		Renderer::BufferWrap mTerrainNodeDescriptorBuffer;	// GPU地形全节点状态表，被主渲染线程与后台线程(每次应该只更新部分节点)访问，该对象类似于Rvt中的PageTable
	};

}