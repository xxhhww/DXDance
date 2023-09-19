#pragma once
#include "Renderer/TileCache.h"

namespace Renderer {

	struct PageLevelTableChunk {
	public:
		int32_t    mipLevel;
		Math::Int4 rect;

		bool inQueue  { false };	// 是否位于任务队列中
		bool inLoading{ false };	// 是否位于GPU加载中
		bool inTexture{ false };	// 是否位于物理纹理中

		TileCache::Node* node{ nullptr };	// 对应的Tile节点

	public:
		inline void SetInActive()	{ inQueue = false; inLoading = false; inTexture = false; }
		inline void SetInQueue()	{ inQueue = true;  inLoading = false; inTexture = false; }
		inline void SetInLoading()	{ inQueue = false; inLoading = true;  inTexture = false; }
		inline void SetInTexture()	{ inQueue = false; inLoading = false; inTexture = true;  }
	};

	struct PageLevelTable {
	public:
		int32_t mipLevel;
		int32_t chunkMipSize;	// PageLevelTable0的ChunkSize为1，其余依次pow2，没有其他含义
		int32_t chunkCountPerAxis;
		int32_t chunkCount;
		std::vector<std::vector<PageLevelTableChunk>> chunks;

	public:
		inline auto& GetChunk(int32_t page0PosX, int32_t page0PosY) {
			page0PosX /= chunkMipSize;
			page0PosY /= chunkMipSize;

			return chunks.at(page0PosX).at(page0PosY);
		}

		inline const auto& GetChunk(int page0PosX, int page0PosY) const {
			page0PosX /= chunkMipSize;
			page0PosY /= chunkMipSize;

			return chunks.at(page0PosX).at(page0PosY);
		}
	};

	struct PageTable {
	public:
		int32_t maxLevel;
		int32_t mostDetailTileCountPerAxis;
		std::vector<PageLevelTable> mPageLevelTables;
		
	public:
		PageTable(int32_t maxLevel, int32_t mostDetailTileCountPerAxis);
		~PageTable() = default;

		inline auto& GetChunk(int page0PosX, int page0PosY, int mipLevel) { 
			return mPageLevelTables.at(mipLevel).GetChunk(page0PosX, page0PosY); 
		}

		inline const auto& GetChunk(int page0PosX, int page0PosY, int mipLevel) const {
			return mPageLevelTables.at(mipLevel).GetChunk(page0PosX, page0PosY);
		}
	};
}