#include "Renderer/GrasslandLinearBufferCache.h"

namespace Renderer {

	GrasslandLinearBufferCache::GrasslandLinearBufferCache(int32_t tileCount) {
		int32_t cacheCount = tileCount;

		mNodes.resize(cacheCount);

		for (int32_t i = 0; i < cacheCount; i++) {
			mNodes[i] = new Node(i);
		}
		for (int32_t i = 0; i < cacheCount; i++) {
			mNodes[i]->next = (i + 1 < cacheCount) ? mNodes[i + 1] : nullptr;
			mNodes[i]->prev = (i != 0) ? mNodes[i - 1] : nullptr;
		}
		mHead = mNodes[0];
		mTail = mNodes[cacheCount - 1];
	}

	GrasslandLinearBufferCache::~GrasslandLinearBufferCache() {
		for (uint32_t i = 0; i < mNodes.size(); i++) {
			delete mNodes[i];
		}

		mNodes.clear();
	}

	// 添加到列表表尾
	void GrasslandLinearBufferCache::AddTail(GrasslandLinearBufferCache::Node* node) {
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

	// 添加到列表表头
	void GrasslandLinearBufferCache::AddHead(GrasslandLinearBufferCache::Node* node) {
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

	// 移除
	void GrasslandLinearBufferCache::Remove(GrasslandLinearBufferCache::Node* node) {
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