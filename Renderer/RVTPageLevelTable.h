#pragma once
#include "Renderer/RvtPageTableNodeCell.h"
#include <vector>
#include <cmath>

namespace Renderer {

	struct RvtPageLevelTable {
	public:
		std::vector<std::vector<RvtPageTableNodeCell>> cells;
		
        Math::Int2 pageOffset;

        int32_t mipLevel { 0 };
        int32_t cellSize { 0 };	// cell大小
        int32_t cellCount{ 0 };	// cell个数

	public:
        RvtPageLevelTable(int32_t mip, int32_t tableSize) {
            pageOffset = Math::Int2{ 0 ,0 };
            mipLevel = mip;
            cellSize = (int32_t)std::pow(2, mipLevel);
            cellCount = tableSize / cellSize;

            cells = std::vector<std::vector<RvtPageTableNodeCell>>{
                (size_t)cellCount, std::vector<RvtPageTableNodeCell>{ (size_t)cellCount, RvtPageTableNodeCell{} } };

            for (int32_t i = 0; i < cellCount; i++) {
                for (int32_t j = 0; j < cellCount; j++) {
                    cells.at(i).at(j) = RvtPageTableNodeCell{ 
                        i * cellSize, j * cellSize, cellSize, cellSize, mipLevel };
                }
            }
        }

        inline auto& GetCell(int32_t x, int32_t y) {
            x /= cellSize;
            y /= cellSize;

            x = (x + pageOffset.x) % cellCount;
            y = (y + pageOffset.y) % cellCount;

            return cells.at(x).at(y);
        }

        inline const auto& GetCell(int32_t x, int32_t y) const {
            x /= cellSize;
            y /= cellSize;

            x = (x + pageOffset.x) % cellCount;
            y = (y + pageOffset.y) % cellCount;

            return cells.at(x).at(y);
        }
	};

}