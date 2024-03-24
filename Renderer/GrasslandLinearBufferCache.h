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
			int32_t    tileIndex{ -1 };			// ��LinearBuffer�ϵ�tile����

			int32_t    grassNodeIndex{ -1 };	// ��ͼ��Ԫ����װ�صĵ��νڵ�����ֵ

		public:
			inline Node(int32_t tileIndex) : tileIndex(tileIndex) {}
			inline ~Node() = default;
		};

	public:
		GrasslandLinearBufferCache(int32_t tileCount);
		~GrasslandLinearBufferCache();

		// ��ӵ��б��β
		void AddTail(GrasslandLinearBufferCache::Node* node);

		// ��ӵ��б��ͷ
		void AddHead(GrasslandLinearBufferCache::Node* node);

		// �Ƴ�
		void Remove(GrasslandLinearBufferCache::Node* node);

		// ��ȡ�б�ͷ�ڵ㣨���Ծ�ڵ㣩
		inline GrasslandLinearBufferCache::Node* GetHead() { return mHead; }

		// ��ȡ�б�β�ڵ㣨���Ծ�ڵ㣩
		inline GrasslandLinearBufferCache::Node* GetTail() { return mTail; }

	public:
		std::vector<GrasslandLinearBufferCache::Node*> mNodes;
		GrasslandLinearBufferCache::Node* mHead{ nullptr };
		GrasslandLinearBufferCache::Node* mTail{ nullptr };
	};

}