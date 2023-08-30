#include "Renderer/RvtPageTable.h"
#include <cmath>

namespace Renderer {

    RvtPageTable::RvtPageTable(uint32_t tableSize) 
    : mTableSize(tableSize)
    , mMaxMipLevel((int)std::log2(tableSize)) {

        mPageLevelTables.resize(mMaxMipLevel + 1u);
        for (uint32_t i = 0; i <= mMaxMipLevel; i++) {
            mPageLevelTables[i] = std::move(RvtPageLevelTable{ i, tableSize });
        }
    }

    RvtPageTable::~RvtPageTable() {
    }

}