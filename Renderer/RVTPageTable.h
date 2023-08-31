#pragma once
#include "Renderer/RvtPageLevelTable.h"
#include <unordered_map>

namespace Renderer {

	class RvtPageTable {
	public:
		RvtPageTable(int32_t tableSize);
		~RvtPageTable();

		inline RvtPageLevelTable& GetPage(int mipLevel) { return mPageLevelTables.at(mipLevel); }
		inline const RvtPageLevelTable& GetPage(int mipLevel) const { return mPageLevelTables.at(mipLevel); }

		inline RvtPageTableNodeCell& GetCell(int x, int y, int mipLevel) { return mPageLevelTables[mipLevel].GetCell(x, y); }
		inline const RvtPageTableNodeCell& GetCell(int x, int y, int mipLevel) const { return mPageLevelTables[mipLevel].GetCell(x, y); }
		
		inline const auto& GetTableSize()   const { return mTableSize; }
		inline const auto& GetMaxMipLevel() const { return mMaxMipLevel; }
		inline const auto& GetCellCount()   const { return mCellCount; }

	private:
		int32_t mTableSize;	// Ò³±í³ß´ç
		int32_t mMaxMipLevel;
		int32_t mCellCount;

		std::vector<RvtPageLevelTable> mPageLevelTables;
	};

}