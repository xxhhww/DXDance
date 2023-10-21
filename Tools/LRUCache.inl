#pragma once
#include "Tools/LRUCache.h"

namespace Tool {

	template<typename LRUCacheNodeUserDataType>
	LRUCache<LRUCacheNodeUserDataType>::LRUCache(int32_t cacheCount) {
		mNodes.resize(cacheCount);

		for (int32_t i = 0; i < cacheCount; i++) {
			mNodes[i] = new LRUCache::Node(i);
		}
		for (int32_t i = 0; i < cacheCount; i++) {
			mNodes[i]->next = (i + 1 < cacheCount) ? mNodes[i + 1] : nullptr;
			mNodes[i]->prev = (i != 0) ? mNodes[i - 1] : nullptr;
		}
		mHead = mNodes[0];
		mTail = mNodes[cacheCount - 1];
	}

	template<typename LRUCacheNodeUserDataType>
	LRUCache<LRUCacheNodeUserDataType>::~LRUCache() {
		for (uint32_t i = 0; i < mNodes.size(); i++) {
			delete mNodes[i];
		}

		mNodes.clear();
	}

	// 添加到列表表尾
	template<typename LRUCacheNodeUserDataType>
	void LRUCache<LRUCacheNodeUserDataType>::AddTail(typename LRUCache<LRUCacheNodeUserDataType>::Node* node) {
		if (node == mTail) {
			return;
		}

		auto* lastTail = mTail;
		lastTail->next = node;
		mTail = node;
		node->prev = lastTail;
	}

	// 添加到列表表头
	template<typename LRUCacheNodeUserDataType>
	void LRUCache<LRUCacheNodeUserDataType>::AddHead(typename LRUCache<LRUCacheNodeUserDataType>::Node* node) {
		if (node == mHead) {
			return;
		}

		auto* lastHead = mHead;
		lastHead->prev = node;
		mHead = node;
		node->next = lastHead;
	}

	// 移除
	template<typename LRUCacheNodeUserDataType>
	void LRUCache<LRUCacheNodeUserDataType>::Remove(typename LRUCache<LRUCacheNodeUserDataType>::Node* node) {
		if (mHead == node) {
			mHead = node->next;
			mHead->prev = nullptr;
		}
		else if (mTail == node) {
			mTail = node->prev;
			mTail->next = nullptr;
		}
		else {
			node->prev->next = node->next;
			node->next->prev = node->prev;
		}
		node->prev = nullptr;
		node->next = nullptr;
	}

}