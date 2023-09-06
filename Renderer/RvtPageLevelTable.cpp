#include "RvtPageLevelTable.h"

namespace Renderer {

    RvtPageLevelTable::RvtPageLevelTable(int32_t mip, int32_t tableSize) {
        pageOffset = Math::Int2{ 0, 0 };
        mipLevel = mip;
        cellSize = (int32_t)std::pow(2, mipLevel);
        cellCount = tableSize / cellSize;

        cells = std::vector<std::vector<RvtPageLevelTableCell>>{
            (size_t)cellCount, std::vector<RvtPageLevelTableCell>{ (size_t)cellCount, RvtPageLevelTableCell{} } };

        for (int32_t i = 0; i < cellCount; i++) {
            for (int32_t j = 0; j < cellCount; j++) {
                cells.at(i).at(j) = std::move(RvtPageLevelTableCell{
                    i * cellSize, j * cellSize, cellSize, cellSize, mipLevel }
                );
            }
        }
    }

	void RvtPageLevelTable::ViewRectChanged(
		Math::Int2 viewRectOffset, 
		const std::function<void(const Math::Int2&)>& retiredFunc) {

        // 纹理全都不可重用
        if (viewRectOffset.x % cellSize != 0 || viewRectOffset.y % cellSize != 0) {
            for (int i = 0; i < cellCount; i++) {
                for (int j = 0; j < cellCount; j++) {
                    const Math::Int2 transXY = GetTransXY(i, j);
                    const auto& cell = cells.at(transXY.x).at(transXY.y);
                    retiredFunc(cell.tilePos);
                }
            }
            pageOffset = Math::Int2{ 0, 0 };
            return;
        }

        // offset是以TableSize * TableSize为单位的, 将offset变化为以当前Table为单位
        viewRectOffset.x /= cellSize;
        viewRectOffset.y /= cellSize;

        if (viewRectOffset.x > 0) {
            for (int i = 0; i < viewRectOffset.x; i++) {
                for (int j = 0; j < cellCount; j++) {
                    const Math::Int2 transXY = GetTransXY(i, j);
                    const auto& cell = cells.at(transXY.x).at(transXY.y);
                    retiredFunc(cell.tilePos);
                }
            }
        }
        else if (viewRectOffset.x < 0) {
            for (int i = 1; i <= -viewRectOffset.x; i++) {
                for (int j = 0; j < cellCount; j++) {
                    const Math::Int2 transXY = GetTransXY(cellCount - i, j);
                    const auto& cell = cells.at(transXY.x).at(transXY.y);
                    retiredFunc(cell.tilePos);
                }
            }
        }
        if (viewRectOffset.y > 0) {
            for (int i = 0; i < viewRectOffset.y; i++) {
                for (int j = 0; j < cellCount; j++) {
                    const Math::Int2 transXY = GetTransXY(j, i);
                    const auto& cell = cells.at(transXY.x).at(transXY.y);
                    retiredFunc(cell.tilePos);
                }
            }
        }
        else if (viewRectOffset.y < 0) {
            for (int i = 1; i <= -viewRectOffset.y; i++) {
                for (int j = 0; j < cellCount; j++) {
                    const Math::Int2 transXY = GetTransXY(j, cellCount - i);
                    const auto& cell = cells.at(transXY.x).at(transXY.y);
                    retiredFunc(cell.tilePos);
                }
            }
        }

        pageOffset += viewRectOffset;
        while (pageOffset.x < 0) {
            pageOffset.x += cellCount;
        }
        while (pageOffset.y < 0) {
            pageOffset.y += cellCount;
        }
        pageOffset.x %= cellCount;
        pageOffset.y %= cellCount;

	}

}