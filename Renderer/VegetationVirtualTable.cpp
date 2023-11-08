#include "Renderer/VegetationVirtualTable.h"
#include "Tools/Assert.h"

namespace Renderer {

	VegetationVirtualTable::VegetationVirtualTable(VegetationDataCache* dataCacher, uint32_t tableCellCountPerAxis, float grassClusterMeterSize, const Math::Vector4& worldMeterSize)
	: mDataCacher(dataCacher)
	, mTableCellCountPerAxis(tableCellCountPerAxis)
	, mGrassClusterRectMeterSize(grassClusterMeterSize) 
	, mWorldMeterSize(worldMeterSize) {
		ASSERT_FORMAT(tableCellCountPerAxis % 2 == 0, "Must be a multiple of 2");
		mGrassClusterVirtualTable.resize(tableCellCountPerAxis, std::vector<std::shared_ptr<VirtualTableCell>>{});
		for (uint32_t i = 0; i < tableCellCountPerAxis; i++) {
			auto& currGrassClusterTableRow = mGrassClusterVirtualTable.at(i);
			for (uint32_t j = 0; j < tableCellCountPerAxis; j++) {
				currGrassClusterTableRow.emplace_back(std::make_shared<VirtualTableCell>());
			}
		}
	}

	VegetationVirtualTable::~VegetationVirtualTable() {

	}

	void VegetationVirtualTable::Update(const Math::Vector2& currCameraPos, std::vector<VirtualTableCell*>& changedVirtualTableCells) {
		const Math::Vector2 currFixedPosXZ = GetFixedPosition(currCameraPos);
		if (mFirstFrame) {
			for (int32_t i = 0; i < mGrassClusterVirtualTable.size(); i++) {
				auto& currGrassClusterTableRow = mGrassClusterVirtualTable.at(i);
				for (int32_t j = 0; j < currGrassClusterTableRow.size(); j++) {
					auto* currGrassClusterTableCell = currGrassClusterTableRow.at(j).get();
					// 更新GrassClusterVirtualTableCell对应的GrassClusterRect
					currGrassClusterTableCell->targetGrassClusterRect = {
						(j - mTableCellCountPerAxis / 2) * mGrassClusterRectMeterSize + currFixedPosXZ.x,
						((mTableCellCountPerAxis - i - 1) - mTableCellCountPerAxis / 2) * mGrassClusterRectMeterSize + currFixedPosXZ.y,
						mGrassClusterRectMeterSize, mGrassClusterRectMeterSize
					};
					if (IsRectValid(currGrassClusterTableCell->targetGrassClusterRect, mWorldMeterSize)) {
						changedVirtualTableCells.push_back(currGrassClusterTableCell);
					}
				}
			}
			mFirstFrame = false;
			mPrevFixedPosXZ = currFixedPosXZ;
		}
		else {
			// 检测固定在网格上的坐标是否已经偏移
			float xDiff = currFixedPosXZ.x - mPrevFixedPosXZ.x;
			float yDiff = currFixedPosXZ.y - mPrevFixedPosXZ.y;
			if (std::abs(xDiff) > 0.0f || std::abs(yDiff) > 0.0f) {
				mPrevFixedPosXZ = currFixedPosXZ;

				// 对GrassClusterVirtualTableCell做更新，并将需要重新烘焙的TableCell取出来
				float uniformDiffX = xDiff / mGrassClusterRectMeterSize;
				float uniformDiffY = yDiff / mGrassClusterRectMeterSize;

				if (uniformDiffX > 0.0f) {
					for (int32_t i = 0; i < mTableCellCountPerAxis; i++) {
						auto* virtualTableCell = GetOffsetVirtualTableCell(i, 0);
						virtualTableCell->targetGrassClusterRect.x += mTableCellCountPerAxis * mGrassClusterRectMeterSize;
						if (IsRectValid(virtualTableCell->targetGrassClusterRect, mWorldMeterSize)) {
							changedVirtualTableCells.push_back(virtualTableCell);
						}
					}
				}
				else if (uniformDiffX < 0.0f) {
					for (int32_t i = 0; i < mTableCellCountPerAxis; i++) {
						auto* virtualTableCell = GetOffsetVirtualTableCell(i, mTableCellCountPerAxis - 1);
						virtualTableCell->targetGrassClusterRect.x -= mTableCellCountPerAxis * mGrassClusterRectMeterSize;
						if (IsRectValid(virtualTableCell->targetGrassClusterRect, mWorldMeterSize)) {
							changedVirtualTableCells.push_back(virtualTableCell);
						}
					}
				}

				if (uniformDiffY > 0.0f) {
					for (int32_t i = 0; i < mTableCellCountPerAxis; i++) {
						auto* virtualTableCell = GetOffsetVirtualTableCell(mTableCellCountPerAxis - 1, i);
						virtualTableCell->targetGrassClusterRect.y += mTableCellCountPerAxis * mGrassClusterRectMeterSize;
						if (IsRectValid(virtualTableCell->targetGrassClusterRect, mWorldMeterSize)) {
							changedVirtualTableCells.push_back(virtualTableCell);
						}
					}
				}
				else if (uniformDiffY < 0.0f) {
					for (int32_t i = 0; i < mTableCellCountPerAxis; i++) {
						auto* virtualTableCell = GetOffsetVirtualTableCell(0, i);
						virtualTableCell->targetGrassClusterRect.y -= mTableCellCountPerAxis * mGrassClusterRectMeterSize;
						if (IsRectValid(virtualTableCell->targetGrassClusterRect, mWorldMeterSize)) {
							changedVirtualTableCells.push_back(virtualTableCell);
						}
					}
				}
			}
		}

		// 为需要重新烘焙的TableCell分配PhysicalData
		for (auto& cell : changedVirtualTableCells) {
			auto* cache = mDataCacher->ActivateGrassClusterCache(cell->targetGrassClusterRect);
			if (cache == nullptr) {
				cache = mDataCacher->GetAvailableGrassClusterCache();
				cache->userData.opGrassClusterRect = cell->targetGrassClusterRect;
			}
			cell->cahce = cache;
		}
	}

	VegetationVirtualTable::VirtualTableCell* VegetationVirtualTable::GetOffsetVirtualTableCell(int32_t x, int32_t y) {
		int32_t offsetX = x - mVirtualTableOffset.x;
		offsetX = offsetX < 0 ? offsetX + (int32_t)mTableCellCountPerAxis : offsetX;
		offsetX = offsetX % mTableCellCountPerAxis;

		int32_t offsetY = y - mVirtualTableOffset.y;
		offsetY = offsetY < 0 ? offsetY + (int32_t)mTableCellCountPerAxis : offsetY;
		offsetY = offsetY % mTableCellCountPerAxis;

		return mGrassClusterVirtualTable[offsetX][offsetY].get();
	}

	bool VegetationVirtualTable::IsRectValid(const Math::Vector4& grassClusterRect, const Math::Vector4& terrainRect) {
		return true;
	}

	const Math::Vector2& VegetationVirtualTable::GetFixedPosition(const Math::Vector2& currPosition) {
		return Math::Vector2{
			std::floor(currPosition.x / mGrassClusterRectMeterSize + 0.5f) * mGrassClusterRectMeterSize,
			std::floor(currPosition.y / mGrassClusterRectMeterSize + 0.5f) * mGrassClusterRectMeterSize
		};
	}

}