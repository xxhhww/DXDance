#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include "HoudiniApi/HoudiniGeoNode.h"
#include "HoudiniApi/HoudiniApiUtility.h"
#include <string>
#include <vector>

namespace Houdini {

	struct HoudiniAsset;
	/*
	* Represents the Houdini Object node.
	*/
	struct HoudiniObjectNode {
	public:
		HoudiniAsset* parentAsset{ nullptr };

		std::string     objectName;
		HAPI_ObjectInfo objectInfo;
		HAPI_Transform  objectTransform;
		std::vector<std::unique_ptr<HoudiniGeoNode>> geoNodes;

	public:

		void Create(const HAPI_Session* session, HoudiniAsset* houdiniAsset, HAPI_ObjectInfo hapiObjectInfo, HAPI_Transform hapiObjectTranform, bool bUseOutputNodes);

	};

}