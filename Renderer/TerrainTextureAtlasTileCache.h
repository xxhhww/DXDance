#pragma once
#include <vector>
#include "Math/Int.h"

namespace Renderer {

	/*
	* 地形纹理图集中TerrainTextureAtlasTileCache
	*/
	class TerrainTextureAtlasTileCache {
	public:
		struct Node {
		public:
			Node* next{ nullptr };
			Node* prev{ nullptr };

		public:
			int32_t    tileID{ -1 };		// 在纹理图集上的ID
			Math::Int2 tilePos{ -1, -1 };	// 在纹理图集上的索引

			int32_t    terrainNodeIndex{ -1 };	// 该图集元素上装载的地形节点索引值
			int32_t    mipLevel{ -1 };			// mipLevel值(lod值)
			Math::Int2 pagePos{ -1, -1 };		// 当前mipLevel下的索引

		public:
			inline Node(int32_t id, const Math::Int2& pos) : tileID(id), tilePos(pos) {}
			inline ~Node() = default;
		};

	public:
		TerrainTextureAtlasTileCache(int32_t cacheCount, int32_t tileCountPerAxis);
		~TerrainTextureAtlasTileCache();

		// 添加到列表表尾
		void AddTail(TerrainTextureAtlasTileCache::Node* node);

		// 添加到列表表头
		void AddHead(TerrainTextureAtlasTileCache::Node* node);

		// 移除
		void Remove(TerrainTextureAtlasTileCache::Node* node);

		// 获取列表头节点（最不活跃节点）
		inline TerrainTextureAtlasTileCache::Node* GetHead() { return mHead; }

		// 获取列表尾节点（最活跃节点）
		inline TerrainTextureAtlasTileCache::Node* GetTail() { return mTail; }

	public:
		std::vector<TerrainTextureAtlasTileCache::Node*> mNodes;
		TerrainTextureAtlasTileCache::Node* mHead{ nullptr };
		TerrainTextureAtlasTileCache::Node* mTail{ nullptr };
	};

}