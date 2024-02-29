#include "Renderer/RuntimeVTPageTable.h"

namespace Renderer {

	RuntimeVTPageTable::RuntimeVTPageTable(uint32_t pageLevel, uint32_t tableSizeInPage0Level)
	: mPageLevel(pageLevel) 
	, mNodeCountPerAxis(tableSizeInPage0Level / std::pow(2, pageLevel)) {
		mNodeRuntimeStates.resize(mNodeCountPerAxis);
		for (int32_t i = 0; i < mNodeCountPerAxis; i++) {

			mNodeRuntimeStates.at(i).resize(mNodeCountPerAxis);
			for (int32_t j = 0; j < mNodeCountPerAxis; j++) {
				auto& nodeRuntimeState = mNodeRuntimeStates.at(i).at(j);
				nodeRuntimeState.pageLevel = mPageLevel;
				nodeRuntimeState.inQueue = false;
				nodeRuntimeState.inLoading = false;
				nodeRuntimeState.inTexture = false;
				nodeRuntimeState.atlasNode = nullptr;
			}
		}
	}

}