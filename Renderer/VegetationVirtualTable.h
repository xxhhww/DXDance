#pragma once
#include "Renderer/VegetationPhysicalDataCache.h"

namespace Renderer {

	/*
	* ��Ϸ����ʱʵ�ʿ��Կ����Ĳݵķ�Χ
	*/
	class VegetationVirtualTable {
	public:
		struct VirtualTableCell {
		public:
			Math::Vector4 targetGrassClusterRect;							// ��VirtaulTableCellӳ���GassClusterRect
			VegetationDataCache::GrassClusterCache::Node* cahce{ nullptr };	// ��VirtualTableCell��Ӧ������Buffer�ϵ�cache
		};

	public:
		VegetationVirtualTable(VegetationDataCache* dataCacher, uint32_t tableCellCountPerAxis, float grassClusterMeterSize, const Math::Vector4& worldMeterSize);
		~VegetationVirtualTable();

		void Update(const Math::Vector2& currCameraPosition, std::vector<VirtualTableCell>& changedVirtualTableCell);

		inline const auto& GetVirtualTable() const { return mGrassClusterVirtualTable; }

	private:
		/*
		* ���ƫ�ƹ����VirtualTableCell
		* x,y : [0, mTableCellCountPerAxis - 1]
		*/
		VirtualTableCell& GetOffsetVirtualTableCell(int32_t x, int32_t y);

		/*
		* ���GrassClusterRect�Ƿ���TerrainRect��
		*/
		bool IsRectValid(const Math::Vector4& grassClusterRect, const Math::Vector4& terrainRect);

		/*
		* ����ǰλ�ù̶�������ڵ�
		*/
		const Math::Vector2& GetFixedPosition(const Math::Vector2& currPosition);

	private:
		bool mFirstFrame{ true };	// ��һ֡ʱ�����е�TableCell��Ҫ����
		Math::Vector2 mPrevFixedPosXZ;

		VegetationDataCache* mDataCacher{ nullptr };
		uint32_t mTableCellCountPerAxis{ 0u };
		float mGrassClusterRectMeterSize{ 0u };
		Math::Vector4 mWorldMeterSize;

		Math::Vector2 mVirtualTableOffset;
		std::vector<std::vector<VirtualTableCell>> mGrassClusterVirtualTable;
	};

}