#include "Renderer/RuntimeVTAtlasTileCache.h"

namespace Renderer {

	RuntimeVTAtlasTileCache::RuntimeVTAtlasTileCache(int32_t tileCountPerAxis) {
		int32_t cacheCount = tileCountPerAxis * tileCountPerAxis;

		mNodes.resize(cacheCount);

		for (int32_t i = 0; i < cacheCount; i++) {
			mNodes[i] = new Node(i, Math::Int2{ i % tileCountPerAxis, i / tileCountPerAxis });
		}
		for (int32_t i = 0; i < cacheCount; i++) {
			mNodes[i]->next = (i + 1 < cacheCount) ? mNodes[i + 1] : nullptr;
			mNodes[i]->prev = (i != 0) ? mNodes[i - 1] : nullptr;
		}
		mHead = mNodes[0];
		mTail = mNodes[cacheCount - 1];
	}

	RuntimeVTAtlasTileCache::~RuntimeVTAtlasTileCache() {
		for (uint32_t i = 0; i < mNodes.size(); i++) {
			delete mNodes[i];
		}

		mNodes.clear();
	}

	void RuntimeVTAtlasTileCache::AddTail(RuntimeVTAtlasTileCache::Node* node) {
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

	void RuntimeVTAtlasTileCache::AddHead(RuntimeVTAtlasTileCache::Node* node) {
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

	void RuntimeVTAtlasTileCache::Remove(RuntimeVTAtlasTileCache::Node* node) {
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