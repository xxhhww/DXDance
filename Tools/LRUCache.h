#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <mutex>
#include "Math/Vector.h"

namespace Tool {

	template<typename LRUCacheNodeUserDataType>
	struct LRUCacheNode {
	public:
		LRUCacheNode<LRUCacheNodeUserDataType>* next{ nullptr };
		LRUCacheNode<LRUCacheNodeUserDataType>* prev{ nullptr };

		uint64_t id;
		LRUCacheNodeUserDataType userData;

	public:
		inline LRUCacheNode(uint64_t nid) : id(nid) {}
		inline ~LRUCacheNode() = default;
	};

	template<typename LRUCacheNodeUserDataType>
	class LRUCache {
	public:
		using Node = LRUCacheNode<LRUCacheNodeUserDataType>;

		LRUCache(int32_t cacheCount);
		~LRUCache();

		// ����LRUCacheNode
		void SetActive(LRUCache::Node* node) { if (node == nullptr) return;  Remove(node); AddTail(node); }

		void Foreach(std::function<void(LRUCache::Node*)>&& lambda) { 
			for (auto& node : mNodes) { 
				lambda(node);
			}
		}

		// ��ȡ�б�ͷ�ڵ㣨���Ծ�ڵ㣩
		inline LRUCache::Node* GetHead() const { return mHead; }

		// ��ȡ�б�β�ڵ㣨���Ծ�ڵ㣩
		inline LRUCache::Node* GetTail() const { return mTail; }

	private:
		// ��ӵ��б��β
		void AddTail(LRUCache::Node* node);

		// ��ӵ��б��ͷ
		void AddHead(LRUCache::Node* node);

		// �Ƴ�
		void Remove(LRUCache::Node* node);
	private:
		std::mutex mMutex;
		std::vector<LRUCache::Node*> mNodes;
		LRUCache::Node* mHead{ nullptr };
		LRUCache::Node* mTail{ nullptr };
	};

}

#include "LRUCache.inl"