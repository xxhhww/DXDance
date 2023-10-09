#include "HoudiniApi/HoudiniApiUtility.h"
#include "Tools/Assert.h"


namespace Houdini {
	void HoudiniApiUtility::SetCookOptions(HAPI_CookOptions& cookOptions) {
		// In keeping consistency with other plugins, we don't support splitting by groups or attributes.
		// Though allowing it now behind an option.
		cookOptions.splitGeosByGroup = false; // HEU_PluginSettings.CookOptionSplitGeosByGroup;
		cookOptions.splitGeosByAttribute = false;
		cookOptions.splitAttrSH = 0;
		cookOptions.splitPointsByVertexAttributes = false;

		cookOptions.cookTemplatedGeos = true; // HEU_PluginSettings.CookTemplatedGeos;
		cookOptions.maxVerticesPerPrimitive = 3; // HEU_PluginSettings.MaxVerticesPerPrimitive;
		cookOptions.refineCurveToLinear = true; // HEU_Defines.HAPI_CURVE_REFINE_TO_LINEAR;
		cookOptions.curveRefineLOD = 8; // HEU_Defines.HAPI_CURVE_LOD;
		cookOptions.packedPrimInstancingMode = HAPI_PACKEDPRIM_INSTANCING_MODE_FLAT;

		cookOptions.handleBoxPartTypes = false; // HEU_PluginSettings.SupportHoudiniBoxType;
		cookOptions.handleSpherePartTypes = false; // HEU_PluginSettings.SupportHoudiniSphereType;
	}

	void HoudiniApiUtility::CookNodeInHoudini(const HAPI_Session* session, HAPI_NodeId nodeID, bool bCookTemplatedGeos, bool bSplitGeosByGroup, std::string assetName) {
		CookNode(session, nodeID, bCookTemplatedGeos, bSplitGeosByGroup);
		ProcessHoudiniCookStatus(session, assetName);
	}

	void HoudiniApiUtility::CookNode(const HAPI_Session* session, HAPI_NodeId nodeID, bool bCookTemplatedGeos, bool bSplitGeosByGroup) {
		HAPI_CookOptions cookOptions{};
		SetCookOptions(cookOptions);
		cookOptions.cookTemplatedGeos = bCookTemplatedGeos;
		cookOptions.splitGeosByGroup |= bSplitGeosByGroup;

		HAPI_Result hapiResult = FHoudiniApi::CookNode(session, nodeID, &cookOptions);
		ASSERT_FORMAT(false, "Cook Node Failed");
	}

	void HoudiniApiUtility::ProcessHoudiniCookStatus(const HAPI_Session* session, std::string assetName) {
		HAPI_Result HAPIResultCode = HAPI_RESULT_SUCCESS;
		HAPI_State HAPIStatusCode = HAPI_STATE_STARTING_LOAD;

		// Busy wait until cooking has finished
		while (HAPIResultCode == HAPI_RESULT_SUCCESS && HAPIStatusCode > HAPI_STATE_MAX_READY_STATE) {
			int32_t stateID = 0;
			HAPIResultCode = FHoudiniApi::GetStatus(session, HAPI_STATUS_COOK_STATE, &stateID);
			HAPIStatusCode = (HAPI_State)stateID;

			/*
			if (HEU_PluginSettings.WriteCookLogs) {
				string cookStatus = session.GetStatusString(HAPI_StatusType.HAPI_STATUS_COOK_STATE, HAPI_StatusVerbosity.HAPI_STATUSVERBOSITY_ERRORS);
				HEU_CookLogs.Instance.AppendCookLog(cookStatus);
			}
			*/
		}

		if (HAPIStatusCode == HAPI_STATE_READY_WITH_COOK_ERRORS) {
			// We should be able to continue even with these errors, but at least notify user.
			std::string statusString = HoudiniApiUtility::GetStatusString(session, HAPI_STATUS_COOK_RESULT, HAPI_STATUSVERBOSITY_WARNINGS);
			ASSERT_FORMAT(false, statusString.c_str());
		}
		else if (HAPIStatusCode == HAPI_STATE_READY_WITH_FATAL_ERRORS) {
			std::string statusString = HoudiniApiUtility::GetStatusString(session, HAPI_STATUS_COOK_RESULT, HAPI_STATUSVERBOSITY_ERRORS);
			ASSERT_FORMAT(false, statusString.c_str());
		}
		else {
			// HEU_Logger.LogFormat("Houdini Engine: Cooking result {0} for asset: {1}", (HAPI_State)statusCode, AssetName);
		}
	}

	void HoudiniApiUtility::GetComposedChildNodeList(const HAPI_Session* session, HAPI_NodeId parentNodeID, HAPI_NodeTypeBits nodeTypeFilter, HAPI_NodeFlagsBits nodeFlagFilter, bool bRecursive, std::vector<HAPI_NodeId>& childNodeIDs) {
		// First compose the internal list and get the count, then get the actual list.
		int count = -1;
		HAPI_Result hapiResult = FHoudiniApi::ComposeChildNodeList(session, parentNodeID, nodeTypeFilter, nodeFlagFilter, bRecursive, &count);
		ASSERT_FORMAT(hapiResult != HAPI_RESULT_SUCCESS, "ComposeChildNodeList Failed");
		if (count > 0) {
			childNodeIDs.resize(count);
			hapiResult = FHoudiniApi::GetComposedChildNodeList(session, parentNodeID, childNodeIDs.data(), count);
			ASSERT_FORMAT(hapiResult != HAPI_RESULT_SUCCESS, "GetComposedChildNodeList Failed");
		}
	}

	void HoudiniApiUtility::GetObjectInfos(const HAPI_Session* session, HAPI_NodeId assetID, const HAPI_NodeInfo& nodeInfo, std::vector<HAPI_ObjectInfo>& objectInfos, std::vector<HAPI_Transform>& objectTransforms) {
		if (nodeInfo.type == HAPI_NODETYPE_SOP) {
			// For SOP assets, we use the parent IDs to get the object info and geo info
			objectInfos.resize(1u);
			HAPI_Result HAPIResultCode = FHoudiniApi::GetObjectInfo(session, nodeInfo.parentId, &objectInfos[0]);
			if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
				ASSERT_FORMAT(false, "GetObjectInfo Failed");
			}

			// Identity transform will be used for SOP assets, so not querying transform
			objectTransforms.resize(1u);
			objectTransforms[0] = HAPI_Transform{};
			FHoudiniApi::Transform_Init(&objectTransforms[0]);
		}
		else if (nodeInfo.type == HAPI_NODETYPE_OBJ) {
			int objectCount = 0;
			HAPI_Result HAPIResultCode = FHoudiniApi::ComposeObjectList(session, assetID, "", &objectCount);
			if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
				ASSERT_FORMAT(false, "ComposeObjectList Failed");
			}

			if (objectCount <= 0) {
				// Since this asset is an object type and has 0 object as children, we use the object itself
				objectInfos.resize(1u);
				HAPIResultCode = FHoudiniApi::GetObjectInfo(session, assetID, &objectInfos[0]);
				if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
					ASSERT_FORMAT(false, "GetObjectInfo Failed");
				}

				// Identity transform will be used for single object assets, so not querying transform
				objectTransforms.resize(1u);
				objectTransforms[0] = HAPI_Transform{};
				FHoudiniApi::Transform_Init(&objectTransforms[0]);
			}
			else {
				// This object has children, so use GetComposedObjectList to get list of HAPI_ObjectInfos
				int immediateSOP = 0;
				HAPIResultCode = FHoudiniApi::ComposeChildNodeList(session, nodeInfo.id, HAPI_NODETYPE_SOP, HAPI_NODEFLAGS_DISPLAY, false, &immediateSOP);
				if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
					ASSERT_FORMAT(false, "ComposeChildNodeList Failed");
				}

				bool addSelf = immediateSOP > 0;

				HAPIResultCode = FHoudiniApi::ComposeObjectList(session, assetID, "", &objectCount);
				if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
					ASSERT_FORMAT(false, "ComposeObjectList Failed");
				}

				if (!addSelf) {
					objectInfos.resize(objectCount);
					objectTransforms.resize(objectCount);
				}
				else {
					objectInfos.resize(objectCount + 1);
					objectTransforms.resize(objectCount + 1);
				}

				if (addSelf) {
					HAPIResultCode = FHoudiniApi::GetObjectInfo(session, nodeInfo.id, &objectInfos[objectCount]);
					if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
						ASSERT_FORMAT(false, "GetObjectInfo Failed");
					}
				}


				if (!HEU_SessionManager.GetComposedObjectListMemorySafe(session, nodeInfo.parentId, objectInfos, 0, objectCount)) {
					return false;
				}

				// Now get the object transforms
				if (!HEU_SessionManager.GetComposedObjectTransformsMemorySafe(session, nodeInfo.parentId, HAPI_SRT, objectTransforms, 0, objectCount)) {
					return false;
				}

				if (addSelf) {
					objectTransforms[objectCount] = HAPI_Transform{};
					FHoudiniApi::Transform_Init(&objectTransforms[objectCount]);
				}
			}
		}
		else {
			ASSERT_FORMAT(false, "Unsupported node type");
		}
	}

	std::string HoudiniApiUtility::GetString(const HAPI_Session* session, HAPI_StringHandle string_handle) {
		int32_t stringBufferLength = 0u;
		HAPI_Result HAPIResultCode = FHoudiniApi::GetStringBufLength(session, string_handle, &stringBufferLength);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "GetStringBufLength Failed");
		}

		std::string string_value;
		string_value.resize(stringBufferLength + 1);
		HAPIResultCode = FHoudiniApi::GetString(session, string_handle, string_value.data(), stringBufferLength);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "GetString Failed");
		}

		return string_value;
	}

	std::string HoudiniApiUtility::GetStatusString(const HAPI_Session* session, HAPI_StatusType status_type, HAPI_StatusVerbosity verbosity) {
		int32_t stringBufferLength = 0u;
		HAPI_Result HAPIResultCode = FHoudiniApi::GetStatusStringBufLength(session, status_type, verbosity, &stringBufferLength);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "GetStatusStringBufLength Failed");
		}

		std::string string_value;
		string_value.resize(stringBufferLength + 1);
		HAPIResultCode = FHoudiniApi::GetStatusString(session, status_type, string_value.data(), stringBufferLength);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "GetStatusString Failed");
		}

		return string_value;
	}

}