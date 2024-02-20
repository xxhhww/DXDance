#pragma once
#include <vector>
#include "Math/Int.h"

namespace Renderer {

	class RuntimeVirtualTextureAtlasTileCache {
	public:
		struct Node {
		public:
			Node* next{ nullptr };
			Node* prev{ nullptr };

		public:
			int32_t    tileID{ -1 };		// ��ͼ���ϵ�ID
			Math::Int2 tilePos{ -1, -1 };	// ��ͼ���ϵ�����

			int32_t    pageLevel{ -1 };		// ����ҳ����
			Math::Int2 pagePos{ -1, -1 };	// ����ҳ�����µ�λ��

		public:
			inline Node(int32_t id, const Math::Int2& pos) : tileID(id), tilePos(pos) {}
			inline ~Node() = default;
		};

	public:
		RuntimeVirtualTextureAtlasTileCache(int32_t tileCountPerAxis);
		~RuntimeVirtualTextureAtlasTileCache();

		// ��ӵ��б��β
		void AddTail(RuntimeVirtualTextureAtlasTileCache::Node* node);

		// ��ӵ��б��ͷ
		void AddHead(RuntimeVirtualTextureAtlasTileCache::Node* node);

		// �Ƴ�
		void Remove(RuntimeVirtualTextureAtlasTileCache::Node* node);

		// ��ȡ�б�ͷ�ڵ㣨���Ծ�ڵ㣩
		inline RuntimeVirtualTextureAtlasTileCache::Node* GetHead() { return mHead; }

		// ��ȡ�б�β�ڵ㣨���Ծ�ڵ㣩
		inline RuntimeVirtualTextureAtlasTileCache::Node* GetTail() { return mTail; }

	public:
		std::vector<RuntimeVirtualTextureAtlasTileCache::Node*> mNodes;
		RuntimeVirtualTextureAtlasTileCache::Node* mHead{ nullptr };
		RuntimeVirtualTextureAtlasTileCache::Node* mTail{ nullptr };
	};

}