#pragma once
#include "Renderer/RvtPageLevelTableCell.h"
#include <vector>
#include <cmath>
#include <functional>

namespace Renderer {

	struct RvtPageLevelTable {
	public:
		std::vector<std::vector<RvtPageLevelTableCell>> cells;
		
        Math::Int2 pageOffset;

        int32_t mipLevel { 0 };
        int32_t cellSize { 0 };	// cell大小
        int32_t cellCount{ 0 };	// cell个数

	public:
        RvtPageLevelTable(int32_t mip, int32_t tableSize);

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

        inline const auto GetTransXY(int x, int y) const
        {
            return Math::Int2((x + pageOffset.x) % cellCount, (y + pageOffset.y) % cellCount);
        }

        /*
        * viewRectOffset以mTableSize为单位
        */
        void ViewRectChanged(Math::Int2 viewRectOffset, const std::function<void(const Math::Int2&)>& retiredFunc);
	};

}