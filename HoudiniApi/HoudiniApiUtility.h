#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include <string>
#include <vector>

namespace Houdini {
	/*
	* 定义一些HoudiniApi的复合操作
	*/
	struct HoudiniApiUtility {
	public:

		static bool HasValidInstanceAttribute(const HAPI_Session* session, HAPI_NodeId geoID, HAPI_PartId partID, std::string attribName);

		static bool GetAttributeInfo(const HAPI_Session* session, HAPI_NodeId geoID, HAPI_PartId partID, std::string attribName, HAPI_AttributeInfo* attribInfo);

		static bool IsSupportedPolygonType(HAPI_PartType partType);

		static void SetCookOptions(HAPI_CookOptions& cookOptions);

		// Cooks nodeand returns true if successfull.
		static void CookNodeInHoudini(const HAPI_Session* session, HAPI_NodeId nodeID, bool bCookTemplatedGeos, bool bSplitGeosByGroup, std::string assetName);

		// Cook the given node. This may trigger cooks on other nodes if connected.
		static void CookNode(const HAPI_Session* session, HAPI_NodeId nodeID, bool bCookTemplatedGeos, bool bSplitGeosByGroup = false);

		// Waits until cooking has finished
		static void ProcessHoudiniCookStatus(const HAPI_Session* session, std::string assetName);

		static void GetComposedChildNodeList(const HAPI_Session* session, HAPI_NodeId parentNodeID, HAPI_NodeTypeBits nodeTypeFilter, HAPI_NodeFlagsBits nodeFlagFilter, bool bRecursive, std::vector<HAPI_NodeId>& childNodeIDs);

		// Gets the object infos and transforms for given asset.
		static void GetObjectInfos(const HAPI_Session* session, HAPI_NodeId assetID, const HAPI_NodeInfo& nodeInfo, std::vector<HAPI_ObjectInfo>& objectInfos, std::vector<HAPI_Transform>& objectTransforms);

		static std::string GetString(const HAPI_Session* session, HAPI_StringHandle string_handle);

		static std::string GetStatusString(const HAPI_Session* session, HAPI_StatusType status_type, HAPI_StatusVerbosity verbosity);

	};
}