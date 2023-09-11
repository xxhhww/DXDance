#pragma once
#include <vector>
#include "Math/Int.h"

namespace Renderer {

	/*
	* CPU端为物理纹理的Tile维护的LRUCache
	*/
	class TileCache {
	public:
		struct Node {
		public:
			Node* next{ nullptr };
			Node* prev{ nullptr };

		public:
			int32_t    tileID  { -1 };		// 在物理纹理上的ID
			Math::Int2 tilePos { -1, -1 };	// 在物理纹理上的索引

			Math::Int2 page0Pos{ -1, -1 };	// 在PageLevelTable0上的位置
			int32_t    mipLevel{ -1 };		// mipLevel值

		public:
			inline Node(int32_t id, const Math::Int2& pos) : tileID(id), tilePos(pos) {}
			~Node() = default;
		};

	public:
		TileCache(int32_t cacheCount, int32_t tileCountPerAxis);
		~TileCache();

		// 添加到列表表尾
		void AddTail(TileCache::Node* node);

		// 添加到列表表头
		void AddHead(TileCache::Node* node);

		// 移除
		void Remove(TileCache::Node* node);

		// 获取列表头节点（最不活跃节点）
		inline TileCache::Node* GetHead() { return mHead; }

		// 获取列表尾节点（最活跃节点）
		inline TileCache::Node* GetTail() { return mTail; }

	private:
		std::vector<TileCache::Node*> mNodes;
		TileCache::Node* mHead{ nullptr };
		TileCache::Node* mTail{ nullptr };
	};

}