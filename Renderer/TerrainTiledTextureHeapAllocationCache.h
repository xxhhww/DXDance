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

		// ��ӵ��б��β
		void AddTail(TerrainTiledTextureHeapAllocationCache::Node* node);

		// ��ӵ��б��ͷ
		void AddHead(TerrainTiledTextureHeapAllocationCache::Node* node);

		// �Ƴ�
		void Remove(TerrainTiledTextureHeapAllocationCache::Node* node);

		// ��ȡ�б�ͷ�ڵ㣨���Ծ�ڵ㣩
		inline TerrainTiledTextureHeapAllocationCache::Node* GetHead() { return mHead; }

		// ��ȡ�б�β�ڵ㣨���Ծ�ڵ㣩
		inline TerrainTiledTextureHeapAllocationCache::Node* GetTail() { return mTail; }

	public:
		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		size_t mHeapAllocationSize{ 0u };
		std::vector<TerrainTiledTextureHeapAllocationCache::Node*> mNodes;
		TerrainTiledTextureHeapAllocationCache::Node* mHead{ nullptr };
		TerrainTiledTextureHeapAllocationCache::Node* mTail{ nullptr };
	};

}