#pragma once
#include <vector>
#include "Math/Int.h"

namespace Renderer {

	/*
	* CPU��Ϊ���������Tileά����LRUCache
	*/
	class TileCache {
	public:
		struct Node {
		public:
			Node* next{ nullptr };
			Node* prev{ nullptr };

		public:
			int32_t    tileID  { -1 };		// �����������ϵ�ID
			Math::Int2 tilePos { -1, -1 };	// �����������ϵ�����

			Math::Int2 page0Pos{ -1, -1 };	// ��PageLevelTable0�ϵ�λ��
			int32_t    mipLevel{ -1 };		// mipLevelֵ

		public:
			inline Node(int32_t id, const Math::Int2& pos) : tileID(id), tilePos(pos) {}
			~Node() = default;
		};

	public:
		TileCache(int32_t cacheCount, int32_t tileCountPerAxis);
		~TileCache();

		// ��ӵ��б��β
		void AddTail(TileCache::Node* node);

		// ��ӵ��б��ͷ
		void AddHead(TileCache::Node* node);

		// �Ƴ�
		void Remove(TileCache::Node* node);

		// ��ȡ�б�ͷ�ڵ㣨���Ծ�ڵ㣩
		inline TileCache::Node* GetHead() { return mHead; }

		// ��ȡ�б�β�ڵ㣨���Ծ�ڵ㣩
		inline TileCache::Node* GetTail() { return mTail; }

	private:
		std::vector<TileCache::Node*> mNodes;
		TileCache::Node* mHead{ nullptr };
		TileCache::Node* mTail{ nullptr };
	};

}