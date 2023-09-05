#pragma once
#include "Renderer/LruCache.h"
#include "Renderer/ResourceAllocator.h"
#include "Math/Int.h"
#include <mutex>

namespace Renderer {

	class RenderEngine;

	class RvtTiledTexture {
	public:
		RvtTiledTexture(RenderEngine* renderEngine);
		~RvtTiledTexture();

		inline const auto& GetTileCountPerAxis() const { return mTileCountPerAxis; }
		inline const auto& GetTileSize()         const { return mTileSize; }
		inline const auto& GetPaddingSize()      const { return mPaddingSize; }
		inline const auto& GetAllTileCount()     const { return mAllTileCount; }
		inline const auto& GetTiledMapSize()     const { return Math::Int2((int32_t)mTiledTextureWidth, (int32_t)mTiledTextureHeight); }

		inline const auto GetTileSizeWithPadding() const { return mTileSize + 2u * mPaddingSize; }

		inline auto& GetTiledMaps() { return mTiledMaps; }
		inline const auto& GetTiledMaps() const { return mTiledMaps; }

		/*
		* ��ȡ���Ա���̭��Tile��TilePos
		*/
		inline const auto RequestTile() const { return IdToPos(mTileCache.GetFirst()); }

		/*
		* ����Ŀ��TileΪ��Ծ
		*/
		inline const auto SetActive(const Math::Int2& tilePos) { return mTileCache.SetActive(PosToId(tilePos)); }

	private:

		inline const Math::Int2 IdToPos(int id)                    const { return Math::Int2(id % mTileCountPerAxis, id / mTileCountPerAxis); }
		inline const int32_t    PosToId(const Math::Int2& tilePos) const { return (tilePos.y * mTileCountPerAxis + tilePos.x); }
	
	private:
		RenderEngine* mRenderEngine{ nullptr };

		uint32_t mTileCountPerAxis  { 15u };	// Tile��UV����ĸ���
		uint32_t mTileSize          { 256u };	// һ��Tile�Ĵ�С
		uint32_t mPaddingSize       { 4u };	// ���ߴ�,ÿ��Tile���������ĸ�����Ҫ������䣬����֧��Ӳ���������

		uint32_t mAllTileCount;

		uint32_t mTiledTextureWidth;
		uint32_t mTiledTextureHeight;

		// ����Դ������Ⱦ�̺߳�RvtProcess�̷߳��ʣ���Ҫ����
		std::mutex mTiledMapLock;
		std::vector<TextureWrap> mTiledMaps;

		// ��̭����
		LruCache mTileCache;
	};

}