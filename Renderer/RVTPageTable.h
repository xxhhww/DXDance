#pragma once
#include "Renderer/RvtPageLevelTable.h"
#include <unordered_map>

namespace Renderer {

	class RvtPageTable {
	public:
		RvtPageTable(uint32_t tableSize);
		~RvtPageTable();

		inline RvtPageTableNodeCell& GetCell(int x, int y, int mipLevel) { mPageLevelTables[mipLevel].GetCell(x, y); }
		inline const RvtPageTableNodeCell& GetCell(int x, int y, int mipLevel) const { mPageLevelTables[mipLevel].GetCell(x, y); }
		
		inline const auto& GetTableSize()   const { return mTableSize; }
		inline const auto& GetMaxMipLevel() const { return mMaxMipLevel; }
	private:
		uint32_t mTableSize;	// Ò³±í³ß´ç

		int mMaxMipLevel;

		std::vector<RvtPageLevelTable> mPageLevelTables;

		std::unordered_map<Math::Int2, RvtPageTableNodeCell> mActivePages;
	};

}