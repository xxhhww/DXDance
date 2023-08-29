#pragma once
#include "Renderer/RVTPageLevelTable.h"
#include <unordered_map>

namespace Renderer {

	class RVTPageTable {
	public:
		RVTPageTable();

		inline const auto& GetTableSize()   const { return mTableSize; }
		inline const auto& GetMaxMipLevel() const { return mMaxMipLevel; }
	private:
		uint32_t mTableSize;	// Ò³±í³ß´ç

		int mMaxMipLevel;

		std::vector<RVTPageLevelTable> mPageLevelTables;

		std::unordered_map<Math::_Int2, _RVTPageTableNodeCell> mActivePages;
	};

}