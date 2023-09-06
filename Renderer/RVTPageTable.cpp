#include "Renderer/RvtPageTable.h"
#include <cmath>

namespace Renderer {

    RvtPageTable::RvtPageTable(int32_t tableSize) 
    : mTableSize(tableSize)
    , mMaxMipLevel((int32_t)std::log2(tableSize)) {

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