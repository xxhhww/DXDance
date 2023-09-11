#pragma once
#include "Renderer/RvtPageLevelTable.h"
#include <unordered_map>

namespace Renderer {

	class RvtPageTable {
	public:
		RvtPageTable(int32_t tableSize, int32_t maxMipLevel);

		/*
		* viewRectOffset以mTableSize为单位
		*/
		void ViewRectChanged(Math::Int2 viewRectOffset, const std::function<void(const Math::Int2&)>& retiredFunc);

		inline RvtPageLevelTable& GetPage(int mipLevel) { return mPageLevelTables.at(mipLevel); }
		inline const RvtPageLevelTable& GetPage(int mipLevel) const { return mPageLevelTables.at(mipLevel); }

		inline RvtPageLevelTableCell& GetCell(int x, int y, int mipLevel) { return mPageLevelTables.at(mipLevel).GetCell(x, y); }
		inline const RvtPageLevelTableCell& GetCell(int x, int y, int mipLevel) const { return mPageLevelTables.at(mipLevel).GetCell(x, y); }
		
		inline const auto& GetTableSize()   const { return mTableSize; }
		inline const auto& GetMaxMipLevel() const { return mMaxMipLevel; }
		inline const auto& GetCellCount()   const { return mCellCount; }

	private:
		int32_t mTableSize;	// 页表尺寸
		int32_t mMaxMipLevel;
		int32_t mCellCount;

		std::vector<RvtPageLevelTable> mPageLevelTables;
	};

}