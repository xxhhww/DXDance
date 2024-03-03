#include "Renderer/RuntimeVTPageTable.h"

namespace Renderer {

	RuntimeVTPageTable::RuntimeVTPageTable(uint32_t pageLevel, uint32_t tableSizeInPage0Level)
	: mPageLevel(pageLevel) 
	, mNodeCountPerAxis((int32_t)tableSizeInPage0Level / std::pow(2, pageLevel)) {

		int32_t powNumber = std::pow(2, mPageLevel);

		mNodeRuntimeStates.resize(mNodeCountPerAxis);
		for (int32_t i = 0; i < mNodeCountPerAxis; i++) {

			mNodeRuntimeStates.at(i).resize(mNodeCountPerAxis);
			for (int32_t j = 0; j < mNodeCountPerAxis; j++) {
				auto& nodeRuntimeState = mNodeRuntimeStates.at(i).at(j);
				nodeRuntimeState.physicalPosX = i;
				nodeRuntimeState.physicalPosY = j;
				nodeRuntimeState.pageLevel = mPageLevel;
				nodeRuntimeState.inQueue = false;
				nodeRuntimeState.inLoading = false;
				nodeRuntimeState.inTexture = false;
				nodeRuntimeState.atlasNode = nullptr;
				nodeRuntimeState.rectInPage0Level = Math::Int4{ i * powNumber, j * powNumber, powNumber, powNumber };
			}
		}
	}

	void RuntimeVTPageTable::OnRuntimeVTRealRectChanged(Math::Int2 offsetInPage0Level, std::vector<RuntimeVTPageTableNodeRuntimeState*>& changedPageNodeRuntimeStates) {
		int32_t powNumber = std::pow(2, mPageLevel);

		if (offsetInPage0Level.x % powNumber != 0 || offsetInPage0Level.y % powNumber != 0) {
			for (int32_t y = 0; y < mNodeCountPerAxis; y++) {
				for (int32_t x = 0; x < mNodeCountPerAxis; x++) {
					const Math::Int2 transformedXY = GetTransformedXY(x, y);
					auto& nodeRuntimeState = mNodeRuntimeStates[transformedXY.x][transformedXY.y];
					if (nodeRuntimeState.atlasNode != nullptr) {
						changedPageNodeRuntimeStates.push_back(&nodeRuntimeState);
					}
					mPageOffset = Math::Int2{ 0, 0 };
				}
			}
			return;
		}

		Math::Int2 offsetInCurrPageLevel = Math::Int2{
			offsetInPage0Level.x / powNumber,
			offsetInPage0Level.y / powNumber
		};

        if (offsetInCurrPageLevel.x > 0) {
            for (int32_t i = 0; i < offsetInCurrPageLevel.x; i++) {
                for (int32_t j = 0; j < mNodeCountPerAxis; j++) {
                    const Math::Int2 transformedXY = GetTransformedXY(i, j);
					auto& nodeRuntimeState = mNodeRuntimeStates[transformedXY.x][transformedXY.y];
					if (nodeRuntimeState.atlasNode != nullptr) {
						changedPageNodeRuntimeStates.push_back(&nodeRuntimeState);
					}
                }
            }
        }
        else if (offsetInCurrPageLevel.x < 0) {
            for (int32_t i = 1; i <= -offsetInCurrPageLevel.x; i++) {
                for (int32_t j = 0; j < mNodeCountPerAxis; j++) {
					const Math::Int2 transformedXY = GetTransformedXY(mNodeCountPerAxis - i, j);
					auto& nodeRuntimeState = mNodeRuntimeStates[transformedXY.x][transformedXY.y];
					if (nodeRuntimeState.atlasNode != nullptr) {
						changedPageNodeRuntimeStates.push_back(&nodeRuntimeState);
					}
                }
            }
        }
        if (offsetInCurrPageLevel.y > 0) {
            for (int32_t i = 0; i < offsetInCurrPageLevel.y; i++) {
                for (int32_t j = 0; j < mNodeCountPerAxis; j++) {
					const Math::Int2 transformedXY = GetTransformedXY(j, i);
					auto& nodeRuntimeState = mNodeRuntimeStates[transformedXY.x][transformedXY.y];
					if (nodeRuntimeState.atlasNode != nullptr) {
						changedPageNodeRuntimeStates.push_back(&nodeRuntimeState);
					}
                }
            }
        }
        else if (offsetInCurrPageLevel.y < 0) {
            for (int32_t i = 1; i <= -offsetInCurrPageLevel.y; i++) {
                for (int32_t j = 0; j < mNodeCountPerAxis; j++) {
					const Math::Int2 transformedXY = GetTransformedXY(j, mNodeCountPerAxis - i);
					auto& nodeRuntimeState = mNodeRuntimeStates[transformedXY.x][transformedXY.y];
					if (nodeRuntimeState.atlasNode != nullptr) {
						changedPageNodeRuntimeStates.push_back(&nodeRuntimeState);
					}
                }
            }
        }

		mPageOffset += offsetInCurrPageLevel;
		while (mPageOffset.x < 0) {
			mPageOffset.x += mNodeCountPerAxis;
		}
		while (mPageOffset.y < 0) {
			mPageOffset.y += mNodeCountPerAxis;
		}
		mPageOffset.x %= mNodeCountPerAxis;
		mPageOffset.y %= mNodeCountPerAxis;
	}

	RuntimeVTPageTableNodeRuntimeState& RuntimeVTPageTable::GetNodeRuntimeStateTransformed(int32_t pagePosX, int32_t pagePosY) {
		const Math::Int2 transformedXY = GetTransformedXY(pagePosX, pagePosY);
		return mNodeRuntimeStates[transformedXY.x][transformedXY.y];
	}

	RuntimeVTPageTableNodeRuntimeState& RuntimeVTPageTable::GetNodeRuntimeStateDirected(int32_t pagePosX, int32_t pagePosY) {
		return mNodeRuntimeStates[pagePosX][pagePosY];
	}

	Math::Int2 RuntimeVTPageTable::GetTransformedXY(int32_t x, int32_t y) {
		return Math::Int2{
			(x + mPageOffset.x) % mNodeCountPerAxis,
			(y + mPageOffset.y) % mNodeCountPerAxis,
		};
	}

}