#include "Renderer/RvtPageTable.h"
#include <cmath>

namespace Renderer {

    RvtPageTable::RvtPageTable(int32_t tableSize, int32_t maxMipLevel)
    : mTableSize(tableSize)
    , mMaxMipLevel(maxMipLevel) {

        for (int32_t i = 0; i <= mMaxMipLevel; i++) {
            mPageLevelTables.emplace_back(i, tableSize);
            mCellCount += mPageLevelTables[i].cellCount;
        }
    }

    void RvtPageTable::ViewRectChanged(
        Math::Int2 viewRectOffset,
        const std::function<void(const Math::Int2&)>& retiredFunc) {

        for (int i = 0; i <= mMaxMipLevel; i++) {
            mPageLevelTables[i].ViewRectChanged(viewRectOffset, retiredFunc);
        }


    }

}