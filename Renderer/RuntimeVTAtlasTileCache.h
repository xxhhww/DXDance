#pragma once
#include <vector>
#include "Math/Int.h"
#include "Math/Vector.h"

namespace Renderer {

	class RuntimeVTAtlasTileCache {
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
			Math::Int2 pageOffset{ -1, -1 };
			Math::Int2 testpagePos{ -1, -1 };
			Math::Vector4 realRect;

		public:
			inline Node(int32_t id, const Math::Int2& pos) : tileID(id), tilePos(pos) {}
			inline ~Node() = default;
		};

	public:
		RuntimeVTAtlasTileCache(int32_t tileCountPerAxis);
		~RuntimeVTAtlasTileCache();

		// ��ӵ��б��β
		void AddTail(RuntimeVTAtlasTileCache::Node* node);

		// ��ӵ��б��ͷ
		void AddHead(RuntimeVTAtlasTileCache::Node* node);

		// �Ƴ�
		void Remove(RuntimeVTAtlasTileCache::Node* node);

		// ��ȡ�б�ͷ�ڵ㣨���Ծ�ڵ㣩
		inline RuntimeVTAtlasTileCache::Node* GetHead() { return mHead; }

		// ��ȡ�б�β�ڵ㣨���Ծ�ڵ㣩
		inline RuntimeVTAtlasTileCache::Node* GetTail() { return mTail; }

	public:
		std::vector<RuntimeVTAtlasTileCache::Node*> mNodes;
		RuntimeVTAtlasTileCache::Node* mHead{ nullptr };
		RuntimeVTAtlasTileCache::Node* mTail{ nullptr };
	};

}