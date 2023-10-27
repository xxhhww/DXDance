#pragma once
#include "Renderer/VegetationPhysicalDataCache.h"

namespace Renderer {

	/*
	* 游戏运行时实际可以看到的草的范围
	*/
	class VegetationVirtualTable {
	public:
		struct VirtualTableCell {
		public:
			Math::Vector4 targetGrassClusterRect;							// 该VirtaulTableCell映射的GassClusterRect
			VegetationDataCache::GrassClusterCache::Node* cahce{ nullptr };	// 该VirtualTableCell对应的物理Buffer上的cache
		};

	public:
		VegetationVirtualTable(VegetationDataCache* dataCacher, uint32_t tableCellCountPerAxis, float grassClusterMeterSize, const Math::Vector4& worldMeterSize);
		~VegetationVirtualTable();

		void Update(const Math::Vector2& currCameraPosition, std::vector<VirtualTableCell>& changedVirtualTableCell);

		inline const auto& GetVirtualTable() const { return mGrassClusterVirtualTable; }

	private:
		/*
		* 获得偏移过后的VirtualTableCell
		* x,y : [0, mTableCellCountPerAxis - 1]
		*/
		VirtualTableCell& GetOffsetVirtualTableCell(int32_t x, int32_t y);

		/*
		* 检测GrassClusterRect是否在TerrainRect中
		*/
		bool IsRectValid(const Math::Vector4& grassClusterRect, const Math::Vector4& terrainRect);

		/*
		* 将当前位置固定到网格节点
		*/
		const Math::Vector2& GetFixedPosition(const Math::Vector2& currPosition);

	private:
		bool mFirstFrame{ true };	// 第一帧时，所有的TableCell都要更新
		Math::Vector2 mPrevFixedPosXZ;

		VegetationDataCache* mDataCacher{ nullptr };
		uint32_t mTableCellCountPerAxis{ 0u };
		float mGrassClusterRectMeterSize{ 0u };
		Math::Vector4 mWorldMeterSize;

		Math::Vector2 mVirtualTableOffset;
		std::vector<std::vector<VirtualTableCell>> mGrassClusterVirtualTable;
	};

}