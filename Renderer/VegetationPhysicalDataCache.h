#pragma once
#include "Renderer/RenderEngine.h"
#include "Renderer/ResourceAllocator.h"
#include "Math/Vector.h"
#include "Tools/LRUCache.h"

namespace Renderer {

	/*
	* 植被系统的数据管理器，负责对显存数据和内存数据进行管理，其他地方则存放数据索引信息
	*/
	class VegetationDataCache {
	public:
		struct GrassClusterCacheNodeData {
		public:
			std::optional<Math::Vector4> opGrassClusterRect = std::nullopt;		// 草群的矩形大小(用来标记当前CacheNode对应的草群位置)
			uint32_t grassBladeBufferIndex;		// 在GrassBladeBuffer中的索引偏移
		};
		using GrassClusterCache = Tool::LRUCache<GrassClusterCacheNodeData>;

		struct GrassMaskTileCacheNodeData {
		public:
			std::optional<Math::Vector4> opGrassMaskTileRect = std::nullopt;	// 标记当前CacheNode对应的GrassMask中的Tile的位置
			Math::Vector2 textureIndex;		// 在GrassMaskPhysicalTexture中的索引
		};
		using GrassMaskCache = Tool::LRUCache<GrassMaskTileCacheNodeData>;

	public:
		VegetationDataCache(RenderEngine* renderEngine);
		~VegetationDataCache() = default;

		// 配置草群Cahce的初始化参数
		void ConfigureGrassClusterCache(uint32_t grassClusterCacheCount, uint32_t maxGrassBladeCountPerCluster);

		// 配置草遮罩Cache的初始化参数
		void ConfigureGrassMaskCache(const std::string& pathname, uint32_t grassMaskCacheCount, uint32_t resolutionPerTile);

		// 激活GrassClusterCache(可能激活失败，这是因为GrassClusterRect对应的Cache已经被其他GrassClusterRect使用了)
		GrassClusterCache::Node* ActivateGrassClusterCache(const Math::Vector4& targetGrassClusterRect);

		// 获取当前可用的GrassClusterCacheNode
		GrassClusterCache::Node* GetAvailableGrassClusterCache();

	private:
		RenderEngine* mRenderEngine{ nullptr };

		std::unique_ptr<GrassClusterCache> mGrassClusterCache;
		BufferWrap mGrassBladeBuffer;

		std::unique_ptr<GrassMaskCache> mGrassLayerMaskCache;
		TextureWrap mGrassLayerMask;
	};

}