#pragma once
#include <vector>
#include "Math/Int.h"
#include "Renderer/BuddyHeapAllocator.h"

namespace Renderer {

	class TerrainTiledTextureHeapAllocationCache {
	public:
		struct Node {
		public:
			Node* next{ nullptr };
			Node* prev{ nullptr };

		public:
			BuddyHeapAllocator::Allocation* heapAllocation{ nullptr };

			int32_t tileIndex{ -1 };

		public:
			inline Node(BuddyHeapAllocator::Allocation* allocation) : heapAllocation(allocation) {}
			inline ~Node() = default;
		};

	public:
		TerrainTiledTextureHeapAllocationCache(int32_t tileCountPerCache, BuddyHeapAllocator* heapAllocator, size_t heapAllocationSize);
		~TerrainTiledTextureHeapAllocationCache();

		// 添加到列表表尾
		void AddTail(TerrainTiledTextureHeapAllocationCache::Node* node);

		// 添加到列表表头
		void AddHead(TerrainTiledTextureHeapAllocationCache::Node* node);

		// 移除
		void Remove(TerrainTiledTextureHeapAllocationCache::Node* node);

		// 获取列表头节点（最不活跃节点）
		inline TerrainTiledTextureHeapAllocationCache::Node* GetHead() { return mHead; }

		// 获取列表尾节点（最活跃节点）
		inline TerrainTiledTextureHeapAllocationCache::Node* GetTail() { return mTail; }

	public:
		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		size_t mHeapAllocationSize{ 0u };
		std::vector<TerrainTiledTextureHeapAllocationCache::Node*> mNodes;
		TerrainTiledTextureHeapAllocationCache::Node* mHead{ nullptr };
		TerrainTiledTextureHeapAllocationCache::Node* mTail{ nullptr };
	};

}