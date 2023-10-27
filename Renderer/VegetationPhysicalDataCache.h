#pragma once
#include "Renderer/RenderEngine.h"
#include "Renderer/ResourceAllocator.h"
#include "Math/Vector.h"
#include "Tools/LRUCache.h"

namespace Renderer {

	/*
	* ֲ��ϵͳ�����ݹ�������������Դ����ݺ��ڴ����ݽ��й��������ط���������������Ϣ
	*/
	class VegetationDataCache {
	public:
		struct GrassClusterCacheNodeData {
		public:
			std::optional<Math::Vector4> opGrassClusterRect = std::nullopt;		// ��Ⱥ�ľ��δ�С(������ǵ�ǰCacheNode��Ӧ�Ĳ�Ⱥλ��)
			uint32_t grassBladeBufferIndex;		// ��GrassBladeBuffer�е�����ƫ��
		};
		using GrassClusterCache = Tool::LRUCache<GrassClusterCacheNodeData>;

		struct GrassMaskTileCacheNodeData {
		public:
			std::optional<Math::Vector4> opGrassMaskTileRect = std::nullopt;	// ��ǵ�ǰCacheNode��Ӧ��GrassMask�е�Tile��λ��
			Math::Vector2 textureIndex;		// ��GrassMaskPhysicalTexture�е�����
		};
		using GrassMaskCache = Tool::LRUCache<GrassMaskTileCacheNodeData>;

	public:
		VegetationDataCache(RenderEngine* renderEngine);
		~VegetationDataCache() = default;

		// ���ò�ȺCahce�ĳ�ʼ������
		void ConfigureGrassClusterCache(uint32_t grassClusterCacheCount, uint32_t maxGrassBladeCountPerCluster);

		// ���ò�����Cache�ĳ�ʼ������
		void ConfigureGrassMaskCache(const std::string& pathname, uint32_t grassMaskCacheCount, uint32_t resolutionPerTile);

		// ����GrassClusterCache(���ܼ���ʧ�ܣ�������ΪGrassClusterRect��Ӧ��Cache�Ѿ�������GrassClusterRectʹ����)
		GrassClusterCache::Node* ActivateGrassClusterCache(const Math::Vector4& targetGrassClusterRect);

		// ��ȡ��ǰ���õ�GrassClusterCacheNode
		GrassClusterCache::Node* GetAvailableGrassClusterCache();

	private:
		RenderEngine* mRenderEngine{ nullptr };

		std::unique_ptr<GrassClusterCache> mGrassClusterCache;
		BufferWrap mGrassBladeBuffer;

		std::unique_ptr<GrassMaskCache> mGrassLayerMaskCache;
		TextureWrap mGrassLayerMask;
	};

}