#include "HoudiniApi/HoudiniObjectNode.h"
#include "Tools/Assert.h"

//这个函数名字和已有的宏冲突了，因此undef
#undef GetGeoInfo

namespace Houdini {

	HoudiniObjectNode::HoudiniObjectNode(const HAPI_Session* session, HoudiniAsset* houdiniAsset, HAPI_ObjectInfo& hapiObjectInfo, HAPI_Transform& hapiObjectTranform, bool bUseOutputNodes) 
	: parentAsset(houdiniAsset)
	, objectInfo(hapiObjectInfo)
	, objectTransform(hapiObjectTranform) {

		objectName = HoudiniApiUtility::GetString(session, objectInfo.nameSH);

		std::vector<HAPI_GeoInfo> geoInfos;

		HAPI_GeoInfo tempGeoInfo{};
		HAPI_Result hapiResult = FHoudiniApi::GetDisplayGeoInfo(session, objectInfo.nodeId, &tempGeoInfo);
		ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetDisplayGeoInfo Failed");
		geoInfos.emplace_back(tempGeoInfo);

		// Get editable nodes, cook em, then create geo nodes for them
		std::vector<HAPI_NodeId> editableNodes;
		HoudiniApiUtility::GetComposedChildNodeList(session, objectInfo.nodeId, (int)HAPI_NODETYPE_SOP, (int)HAPI_NODEFLAGS_EDITABLE, true, editableNodes);
		for (auto& editNodeID : editableNodes) {
			if (editNodeID != tempGeoInfo.nodeId) {

				HoudiniApiUtility::CookNode(session, editNodeID, false);

				HAPI_GeoInfo editGeoInfo{};
				hapiResult = FHoudiniApi::GetGeoInfo(session, editNodeID, &editGeoInfo);
				ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetGeoInfo Failed");
				geoInfos.emplace_back(editGeoInfo);
			}
		}

		int32_t numGeoInfos = geoInfos.size();
		for (int32_t i = 0; i < numGeoInfos; ++i) {
			// Create GeoNode for each
			geoNodes.emplace_back(std::make_unique<HoudiniGeoNode>(session, this, geoInfos[i]));
		}
	}

	void HoudiniObjectNode::GenerateGeometry(const HAPI_Session* session, bool bRebuild) {

	}

	void HoudiniObjectNode::GeneratePartInstances(const HAPI_Session* session) {
		for (auto& geoNode : geoNodes) {
			geoNode->GeneratePartInstances(session);
		}
	}

	void HoudiniObjectNode::GenerateObjectInstances(const HAPI_Session* session) {

	}

	bool HoudiniObjectNode::IsInstancer() {
		if (objectInfo.isInstancer) {
			return true;
		}
		else {
			// Check parts for atrrib instancing
			for (const auto& geoNode : geoNodes) {
				if (geoNode->HasAttribInstancer()) {
					return true;
				}
			}
		}
		return false;
	}

}