#include "HoudiniApi/HoudiniAsset.h"
#include "Tools/Assert.h"
#include <iostream>

namespace Houdini {

	HoudiniAsset::HoudiniAsset(const HAPI_Session* session, const std::string& hdaFilepath, bool bPromptForSubasset, int32_t desiredSubassetIndex) {
		// Load given asset file in Houdini Engine
		HAPI_AssetLibraryId libraryID;
		HAPI_Result hapiResult = FHoudiniApi::LoadAssetLibraryFromFile(session, hdaFilepath.c_str(), false, &libraryID);
		ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "LoadAssetLibraryFromFile Failed");

		// Get the number of assets contained in an asset library
		int assetCount = 0;
		hapiResult = FHoudiniApi::GetAvailableAssetCount(session, libraryID, &assetCount);
		ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetAvailableAssetCount Failed");

		// Get the names of the assets contained in given asset library
		std::vector<HAPI_StringHandle> hapiAssetNames(assetCount);
		hapiResult = FHoudiniApi::GetAvailableAssets(session, libraryID, hapiAssetNames.data(), assetCount);
		ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetAvailableAssets Failed");

		// Convert HAPI_StringHandle to std::string
		subAssetNames.resize(assetCount);
		for (int32_t i = 0; i < assetCount; i++) {
			int32_t stringBufferLength = 0u;
			hapiResult = FHoudiniApi::GetStringBufLength(session, hapiAssetNames[i], &stringBufferLength);
			ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetStringBufLength Failed");

			subAssetNames[i].resize(stringBufferLength + 1);
			hapiResult = FHoudiniApi::GetString(session, hapiAssetNames[i], subAssetNames[i].data(), stringBufferLength);
			ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetString Failed");
		}
	}

	void HoudiniAsset::Build(const HAPI_Session* session) {
		// Load and cook the HDA
		std::string topName = subAssetNames[selectedSubassetIndex];
		HAPI_Result hapiResult = FHoudiniApi::CreateNode(session, -1, topName.c_str(), "", false, &assetID);
		if (hapiResult != HAPI_RESULT_SUCCESS) {
			// Delete node if create failed
			hapiResult = FHoudiniApi::DeleteNode(session, assetID);
			ASSERT_FORMAT(false, "CreateNode Failed");
		}

		// Waits until cooking has finished
		HoudiniApiUtility::ProcessHoudiniCookStatus(session, assetName);

		// In case the cooking wasn't done previously, force it now.
		HoudiniApiUtility::CookNodeInHoudini(session, assetID, true, false, assetName);

		// Get the asset ID
		hapiResult = FHoudiniApi::GetAssetInfo(session, assetID, &assetInfo);
		ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetAssetInfo Failed");

		// Check for any errors
		HAPI_ErrorCodeBits errors{};
		FHoudiniApi::CheckForSpecificErrors(session, assetID, HAPI_ERRORCODE_ASSET_DEF_NOT_FOUND, &errors);
		if (errors > 0) {
			std::cout << "Asset Missing Sub-asset Definitions" << std::endl;
			/*
			"There are undefined nodes. This is due to not being able to find specific " +
			"asset definitions. You might need to load other (dependent) HDAs first."
			*/
		}

		hapiResult = FHoudiniApi::GetNodeInfo(session, assetID, &nodeInfo);
		ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetNodeInfo Failed");

		hapiResult = FHoudiniApi::GetAssetInfo(session, assetID, &assetInfo);
		ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetAssetInfo Failed");

		hapiResult = FHoudiniApi::GetTotalCookCount(
			session,
			assetID,
			(int)(HAPI_NODETYPE_OBJ | HAPI_NODETYPE_SOP),
			(int)(HAPI_NODEFLAGS_OBJ_GEOMETRY | HAPI_NODEFLAGS_DISPLAY),
			true,
			&totalCookCount);
		ASSERT_FORMAT(hapiResult == HAPI_RESULT_SUCCESS, "GetTotalCookCount Failed");

		assetName = HoudiniApiUtility::GetString(session, assetInfo.nameSH);
		assetOpName = HoudiniApiUtility::GetString(session, assetInfo.fullOpNameSH);
		assetHelp = HoudiniApiUtility::GetString(session, assetInfo.helpTextSH);

		// TODO... 生成参数

		ProcessObjectNode(session);

		// TODO... 生成网格

		GeneratePartInstances(session);
	}

	void HoudiniAsset::ProcessObjectNode(const HAPI_Session* session) {
		std::vector<HAPI_ObjectInfo> objectInfos;
		std::vector<HAPI_Transform>  objectTransforms;
		HoudiniApiUtility::GetObjectInfos(session, assetID, nodeInfo, objectInfos, objectTransforms);

		for (uint32_t i = 0; i < objectInfos.size(); i++) {
			objectNodes.emplace_back(std::make_unique<HoudiniObjectNode>(session, this, objectInfos[i], objectTransforms[i], useOutputNodes));
		}
	}

	void HoudiniAsset::GenerateGeometry(const HAPI_Session* session, bool bRebuild) {
		for (auto& objectNode : objectNodes) {
			objectNode->GenerateGeometry(session, bRebuild);
		}
	}

	void HoudiniAsset::GeneratePartInstances(const HAPI_Session* session) {
		// Instancing - process part instances first, then do object instances.
		// This assures that if objects being instanced have all their parts completed.

		// Clear part instances, to make sure that the object instances don't overwrite the part instances.
		for (auto& objectNode : objectNodes) {
			objectNode->GeneratePartInstances(session);
		}

		for (auto& objectNode : objectNodes) {
			if (objectNode->IsInstancer()) {
				objectNode->GenerateObjectInstances(session);
			}
		}
	}

}