#pragma once
#include <vector>
#include "Math/Int.h"

namespace Renderer {

	/*
	* ��������ͼ����TerrainTextureAtlasTileCache
	*/
	class TerrainTextureAtlasTileCache {
	public:
		struct Node {
		public:
			Node* next{ nullptr };
			Node* prev{ nullptr };

		public:
			int32_t    tileID{ -1 };		// ������ͼ���ϵ�ID
			Math::Int2 tilePos{ -1, -1 };	// ������ͼ���ϵ�����

			int32_t    terrainNodeIndex{ -1 };	// ��ͼ��Ԫ����װ�صĵ��νڵ�����ֵ
			int32_t    mipLevel{ -1 };			// mipLevelֵ(lodֵ)
			Math::Int2 pagePos{ -1, -1 };		// ��ǰmipLevel�µ�����

		public:
			inline Node(int32_t id, const Math::Int2& pos) : tileID(id), tilePos(pos) {}
			inline ~Node() = default;
		};

	public:
		TerrainTextureAtlasTileCache(int32_t cacheCount, int32_t tileCountPerAxis);
		~TerrainTextureAtlasTileCache();

		// ��ӵ��б��β
		void AddTail(TerrainTextureAtlasTileCache::Node* node);

		// ��ӵ��б��ͷ
		void AddHead(TerrainTextureAtlasTileCache::Node* node);

		// �Ƴ�
		void Remove(TerrainTextureAtlasTileCache::Node* node);

		// ��ȡ�б�ͷ�ڵ㣨���Ծ�ڵ㣩
		inline TerrainTextureAtlasTileCache::Node* GetHead() { return mHead; }

		// ��ȡ�б�β�ڵ㣨���Ծ�ڵ㣩
		inline TerrainTextureAtlasTileCache::Node* GetTail() { return mTail; }

	public:
		std::vector<TerrainTextureAtlasTileCache::Node*> mNodes;
		TerrainTextureAtlasTileCache::Node* mHead{ nullptr };
		TerrainTextureAtlasTileCache::Node* mTail{ nullptr };
	};

}