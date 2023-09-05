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
		* 获取可以被淘汰的Tile的TilePos
		*/
		inline const auto RequestTile() const { return IdToPos(mTileCache.GetFirst()); }

		/*
		* 设置目标Tile为活跃
		*/
		inline const auto SetActive(const Math::Int2& tilePos) { return mTileCache.SetActive(PosToId(tilePos)); }

	private:

		inline const Math::Int2 IdToPos(int id)                    const { return Math::Int2(id % mTileCountPerAxis, id / mTileCountPerAxis); }
		inline const int32_t    PosToId(const Math::Int2& tilePos) const { return (tilePos.y * mTileCountPerAxis + tilePos.x); }
	
	private:
		RenderEngine* mRenderEngine{ nullptr };

		uint32_t mTileCountPerAxis  { 15u };	// Tile在UV方向的个数
		uint32_t mTileSize          { 256u };	// 一个Tile的大小
		uint32_t mPaddingSize       { 4u };	// 填充尺寸,每个Tile上下左右四个方向都要进行填充，用来支持硬件纹理过滤

		uint32_t mAllTileCount;

		uint32_t mTiledTextureWidth;
		uint32_t mTiledTextureHeight;

		// 该资源被主渲染线程和RvtProcess线程访问，需要加锁
		std::mutex mTiledMapLock;
		std::vector<TextureWrap> mTiledMaps;

		// 淘汰机制
		LruCache mTileCache;
	};

}