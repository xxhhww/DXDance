#pragma once
#include <vector>
#include "Math/Int.h"

namespace Renderer {

	class RuntimeVirtualTextureAtlasTileCache {
	public:
		struct Node {
		public:
			Node* next{ nullptr };
			Node* prev{ nullptr };

		public:
			int32_t    tileID{ -1 };		// 在图集上的ID
			Math::Int2 tilePos{ -1, -1 };	// 在图集上的索引

			int32_t    pageLevel{ -1 };		// 所处页表级别
			Math::Int2 pagePos{ -1, -1 };	// 所处页表级别下的位置

		public:
			inline Node(int32_t id, const Math::Int2& pos) : tileID(id), tilePos(pos) {}
			inline ~Node() = default;
		};

	public:
		RuntimeVirtualTextureAtlasTileCache(int32_t tileCountPerAxis);
		~RuntimeVirtualTextureAtlasTileCache();

		// 添加到列表表尾
		void AddTail(RuntimeVirtualTextureAtlasTileCache::Node* node);

		// 添加到列表表头
		void AddHead(RuntimeVirtualTextureAtlasTileCache::Node* node);

		// 移除
		void Remove(RuntimeVirtualTextureAtlasTileCache::Node* node);

		// 获取列表头节点（最不活跃节点）
		inline RuntimeVirtualTextureAtlasTileCache::Node* GetHead() { return mHead; }

		// 获取列表尾节点（最活跃节点）
		inline RuntimeVirtualTextureAtlasTileCache::Node* GetTail() { return mTail; }

	public:
		std::vector<RuntimeVirtualTextureAtlasTileCache::Node*> mNodes;
		RuntimeVirtualTextureAtlasTileCache::Node* mHead{ nullptr };
		RuntimeVirtualTextureAtlasTileCache::Node* mTail{ nullptr };
	};

}