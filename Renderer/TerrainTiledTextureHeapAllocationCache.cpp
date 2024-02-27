#include "Renderer/TerrainTiledTextureHeapAllocationCache.h"

namespace Renderer {

	TerrainTiledTextureHeapAllocationCache::TerrainTiledTextureHeapAllocationCache(int32_t tileCountPerCache, BuddyHeapAllocator* heapAllocator, size_t heapAllocationSize)
	: mHeapAllocator(heapAllocator)
	, mHeapAllocationSize(heapAllocationSize) {
		int32_t cacheCount = tileCountPerCache;

		mNodes.resize(cacheCount);

		for (int32_t i = 0; i < cacheCount; i++) {
			auto* heapAllocation = mHeapAllocator->AllocateEx(mHeapAllocationSize);
			mNodes[i] = new Node(heapAllocation);
		}
		for (int32_t i = 0; i < cacheCount; i++) {
			mNodes[i]->next = (i + 1 < cacheCount) ? mNodes[i + 1] : nullptr;
			mNodes[i]->prev = (i != 0) ? mNodes[i - 1] : nullptr;
		}
		mHead = mNodes[0];
		mTail = mNodes[cacheCount - 1];
	}

	TerrainTiledTextureHeapAllocationCache::~TerrainTiledTextureHeapAllocationCache() {
		for (uint32_t i = 0; i < mNodes.size(); i++) {
			mHeapAllocator->DeallocateEx(mNodes[i]->heapAllocation);
			delete mNodes[i];
		}

		mNodes.clear();
	}

	void TerrainTiledTextureHeapAllocationCache::AddTail(TerrainTiledTextureHeapAllocationCache::Node* node) {
		if (node == mTail) {
			return;
		}

		if (mTail != nullptr) {
			auto* lastTail = mTail;
			lastTail->next = node;
			mTail = node;
			node->prev = lastTail;
		}
		else {
			mTail = node;
			mHead = node;
		}
	}

	void TerrainTiledTextureHeapAllocationCache::AddHead(TerrainTiledTextureHeapAllocationCache::Node* node) {
		if (node == mHead) {
			return;
		}

		if (mHead != nullptr) {
			auto* lastHead = mHead;
			lastHead->prev = node;
			mHead = node;
			node->next = lastHead;
		}
		else {
			mHead = node;
			mTail = node;
		}
	}

	void TerrainTiledTextureHeapAllocationCache::Remove(TerrainTiledTextureHeapAllocationCache::Node* node) {
		if (mHead == node) {
			mHead = node->next;

			if (mHead != nullptr) {
				mHead->prev = nullptr;
			}
			else {
				mTail = nullptr;
			}
		}
		else if (mTail == node) {
			mTail = node->prev;

			if (mTail != nullptr) {
				mTail->next = nullptr;
			}
			else {
				mHead = nullptr;
			}
		}
		else {
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}
		node->prev = nullptr;
		node->next = nullptr;
	}

}