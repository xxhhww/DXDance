#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include <string>

namespace Houdini {

	struct HoudiniAsset {
	public:
		std::string assetName;
		HAPI_AssetInfo assetInfo;
		HAPI_NodeId nodeID;
		HAPI_NodeInfo nodeInfo;
	};

}