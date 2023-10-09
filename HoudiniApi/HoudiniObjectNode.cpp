#include "HoudiniApi/HoudiniObjectNode.h"
#include "Tools/Assert.h"

//这个函数名字和已有的宏冲突了，因此undef
#undef GetGeoInfo

namespace Houdini {

	void HoudiniObjectNode::Create(const HAPI_Session* session, HoudiniAsset* houdiniAsset, HAPI_ObjectInfo hapiObjectInfo, HAPI_Transform hapiObjectTranform, bool bUseOutputNodes) {
		parentAsset = houdiniAsset;
		objectInfo = hapiObjectInfo;
		objectTransform = hapiObjectTranform;

		objectName = HoudiniApiUtility::GetString(session, objectInfo.nameSH);

		std::vector<HAPI_GeoInfo> geoInfos;

		HAPI_GeoInfo tempGeoInfo{};
		HAPI_Result hapiResult = FHoudiniApi::GetDisplayGeoInfo(session, objectInfo.nodeId, &tempGeoInfo);
		ASSERT_FORMAT(hapiResult != HAPI_RESULT_SUCCESS, "GetDisplayGeoInfo Failed");
		geoInfos.emplace_back(tempGeoInfo);

		// Get editable nodes, cook em, then create geo nodes for them
		std::vector<HAPI_NodeId> childNodeIDs;
		HoudiniApiUtility::GetComposedChildNodeList(session, objectInfo.nodeId, (int)HAPI_NODETYPE_SOP, (int)HAPI_NODEFLAGS_EDITABLE, true, childNodeIDs);
		for (auto& editNodeID : childNodeIDs) {
			if (editNodeID != tempGeoInfo.nodeId) {

				HoudiniApiUtility::CookNode(session, editNodeID, false);

				HAPI_GeoInfo editGeoInfo{};
				hapiResult = FHoudiniApi::GetGeoInfo(session, editNodeID, &editGeoInfo);
				ASSERT_FORMAT(hapiResult != HAPI_RESULT_SUCCESS, "GetGeoInfo Failed");
				geoInfos.emplace_back(editGeoInfo);
			}
		}

		int numGeoInfos = geoInfos.size();
		for (int i = 0; i < numGeoInfos; ++i) {
			// Create GeoNode for each
			_geoNodes.Add(CreateGeoNode(session, geoInfos[i]));
		}
	}

}