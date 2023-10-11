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
		HoudiniObjectNode(const HAPI_Session* session, HoudiniAsset* houdiniAsset, HAPI_ObjectInfo& hapiObjectInfo, HAPI_Transform& hapiObjectTranform, bool bUseOutputNodes);

		void GenerateGeometry(const HAPI_Session* session, bool bRebuild = true);

		void GeneratePartInstances(const HAPI_Session* session);

		void GenerateObjectInstances(const HAPI_Session* session);

		inline bool IsInstanced() { return objectInfo.isInstanced; }

		inline bool IsVisible() { return objectInfo.isVisible; }

		// Returns true if this is an object instancer, or if it has point(attribute) instancer parts.
		bool IsInstancer();
	};

}