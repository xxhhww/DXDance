#pragma once
#include <vector>

namespace Renderer {

	class GrasslandLinearBufferCache {
	public:
		struct Node {
		public:
			Node* next{ nullptr };
			Node* prev{ nullptr };

		public:
			int32_t    tileIndex{ -1 };			// 在LinearBuffer上的tile索引

			int32_t    grassNodeIndex{ -1 };	// 该图集元素上装载的地形节点索引值

		public:
			inline Node(int32_t tileIndex) : tileIndex(tileIndex) {}
			inline ~Node() = default;
		};

	public:
		GrasslandLinearBufferCache(int32_t tileCount);
		~GrasslandLinearBufferCache();

		// 添加到列表表尾
		void AddTail(GrasslandLinearBufferCache::Node* node);

		// 添加到列表表头
		void AddHead(GrasslandLinearBufferCache::Node* node);

		// 移除
		void Remove(GrasslandLinearBufferCache::Node* node);

		// 获取列表头节点（最不活跃节点）
		inline GrasslandLinearBufferCache::Node* GetHead() { return mHead; }

		// 获取列表尾节点（最活跃节点）
		inline GrasslandLinearBufferCache::Node* GetTail() { return mTail; }

	public:
		std::vector<GrasslandLinearBufferCache::Node*> mNodes;
		GrasslandLinearBufferCache::Node* mHead{ nullptr };
		GrasslandLinearBufferCache::Node* mTail{ nullptr };
	};

}