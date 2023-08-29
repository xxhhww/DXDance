#pragma once
#include "Renderer/RVTPageTableNodeCell.h"
#include <vector>
#include <cmath>

namespace Renderer {

	struct RVTPageLevelTable {
	public:
		std::vector<std::vector<RVTPageTableNodeCell>> cells;
		
        Math::Int2 pageOffset;

        int mipLevel { 0 };
        int cellSize { 0 };	// cell大小
        int cellCount{ 0 };	// cell个数

	private:
        RVTPageLevelTable(int mip, int tableSize) {
            pageOffset = Math::Int2{ 0 ,0 };
            mipLevel = mip;
            cellSize = (int)std::pow(2, mipLevel);
            cellCount = tableSize / cellSize;

            cells = std::vector<std::vector<RVTPageTableNodeCell>>{
                cellCount, std::vector<RVTPageTableNodeCell>{ cellCount, RVTPageTableNodeCell{} } };

            for (int i = 0; i < cellCount; i++) {
                for (int j = 0; j < cellCount; j++) {
                    cells.at(i).at(j) = RVTPageTableNodeCell{ 
                        i * cellSize, j * cellSize, cellSize, cellSize, mipLevel };
                }
            }
        }

        RVTPageTableNodeCell GetCell(int x, int y) {
            x /= cellSize;
            y /= cellSize;

            x = (x + pageOffset.x) % cellCount;
            y = (y + pageOffset.y) % cellCount;

            return cells.at(x).at(y);
        }
	};
}