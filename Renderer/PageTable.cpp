#include "Renderer/PageTable.h"

namespace Renderer {

	PageTable::PageTable(int32_t maxLevel, int32_t mostDetailTileCountPerAxis)
	: maxLevel(maxLevel)
	, mostDetailTileCountPerAxis(mostDetailTileCountPerAxis) {
		mPageLevelTables.resize(maxLevel);
		
		int32_t currDetailTileCountPerAxis = mostDetailTileCountPerAxis;
		int32_t currDetailTileSize = 1;		// 当前细节尺度下单个Tile的大小
		for (int32_t mipIndex = 0; mipIndex <= maxLevel; mipIndex++) {
			currDetailTileSize = std::pow(2, mipIndex);

			auto& pageLevelTable = mPageLevelTables.at(mipIndex);
			pageLevelTable.mipLevel = mipIndex;
			pageLevelTable.chunkMipSize = currDetailTileSize;
			pageLevelTable.chunkCountPerAxis = currDetailTileCountPerAxis;
			pageLevelTable.chunkCount = currDetailTileCountPerAxis * currDetailTileCountPerAxis;

			// 填充chunks
			pageLevelTable.chunks.resize(currDetailTileCountPerAxis);
			for (int32_t i = 0; i < currDetailTileCountPerAxis; i++) {
				
				pageLevelTable.chunks.at(i).resize(currDetailTileCountPerAxis);
				for (int32_t j = 0; j < currDetailTileCountPerAxis; j++) {
					auto& chunk = pageLevelTable.chunks.at(i).at(j);
					chunk.mipLevel = mipIndex;
					chunk.rect = Math::Int4{ i * currDetailTileSize, j * currDetailTileSize, currDetailTileSize, currDetailTileSize };
					chunk.inQueue = false;
					chunk.inLoading = false;
					chunk.inTexture = false;
					chunk.node = nullptr;
				}
			}

			currDetailTileCountPerAxis /= 2;
		}
	}

}