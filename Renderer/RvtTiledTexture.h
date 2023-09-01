#pragma once
#include "Renderer/LruCache.h"
#include "Renderer/ResourceAllocator.h"
#include "Math/Int.h"
#include <mutex>

namespace Renderer {

	class RenderEngine;
	class TerrainSystem;

	class RvtTiledTexture {
	public:
		RvtTiledTexture(TerrainSystem* terrainSystem);
		~RvtTiledTexture();

		inline const auto& GetTileCount()           const { return mTileCount; }
		inline const auto& GetTileSize()            const { return mTileSize; }
		inline const auto& GetPaddingSize()         const { return mPaddingSize; }

		inline const auto GetTileSizeWithPadding() const { return mTileSize + 2u * mPaddingSize; }

		/*
		* ��ȡ���Ա���̭��Tile��TilePos
		*/
		inline const auto RequestTile() const { return IdToPos(mTileCache.GetFirst()); }

		/*
		* ����Ŀ��TileΪ��Ծ
		*/
		inline const auto SetActive(const Math::Int2& tilePos) { return mTileCache.SetActive(PosToId(tilePos)); }

	private:

		inline const Math::Int2 IdToPos(int id)                    const { return Math::Int2(id % mTileCount, id / mTileCount); }
		inline const int32_t    PosToId(const Math::Int2& tilePos) const { return (tilePos.y * mTileCount + tilePos.x); }
	
	private:
		RenderEngine* mRenderEngine{ nullptr };
		TerrainSystem* mTerrainSystem{ nullptr };

		uint32_t mTileCount  { 15u };	// Tile��UV����ĸ���
		uint32_t mTileSize   { 256u };	// һ��Tile�Ĵ�С
		uint32_t mPaddingSize{ 4u };	// ���ߴ�,ÿ��Tile���������ĸ�����Ҫ������䣬����֧��Ӳ���������

		uint32_t mTiledTextureWidth;
		uint32_t mTiledTextureHeight;

		// ����Դ������Ⱦ�̺߳�RvtProcess�̷߳��ʣ���Ҫ����
		std::mutex mTiledMapLock;
		std::vector<TextureWrap> mTiledMaps;

		// ��̭����
		LruCache mTileCache;
	};

}