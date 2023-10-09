#include "OfflineTask/ResolveHDAFile.h"
#include "Tools/Assert.h"
#include <iostream>

namespace OfflineTask {

	void ResolveHDAFile::Run(const std::string& filepath) {
		HAPI_Result HAPIResultCode = HAPI_RESULT_FAILURE;

		if (mSession == nullptr) {
			mSession = std::make_unique<HAPI_Session>();
			HAPIResultCode = FHoudiniApi::CreateInProcessSession(mSession.get());
			if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
				ASSERT_FORMAT(false, "CreateInProcessSession Failed");
			}
		}

		HAPI_CookOptions cookOptions{};
		FHoudiniApiUtility::SetCookOptions(cookOptions);

		std::string HDASearchPath = ""; // = HEU_Platform.GetAllFoldersInPath(HEU_Defines.HEU_ENGINE_ASSETS + "/HDAs");
		std::string DSOSearchPath = ""; // = HEU_Platform.GetAllFoldersInPath(HEU_Defines.HEU_ENGINE_ASSETS + "/DSOs");
		std::string environmentFilePath = ""; // = HEU_Platform.GetHoudiniEngineEnvironmentFilePathFull();

		HAPIResultCode = FHoudiniApi::Initialize(mSession.get(), &cookOptions, true, -1, environmentFilePath.c_str(), HDASearchPath.c_str(), DSOSearchPath.c_str(), DSOSearchPath.c_str(), DSOSearchPath.c_str());
		if (HAPIResultCode != HAPI_RESULT_ALREADY_INITIALIZED && HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "Initialize Failed");
		}

		// Load given asset file in Houdini Engine
		HAPI_AssetLibraryId libraryID;
		HAPIResultCode = FHoudiniApi::LoadAssetLibraryFromFile(mSession.get(), filepath.c_str(), true, &libraryID);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "LoadAssetLibraryFromFile Failed");
		}

		// Get the number of assets contained in an asset library
		int assetCount = 0;
		HAPIResultCode = FHoudiniApi::GetAvailableAssetCount(mSession.get(), libraryID, &assetCount);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS || assetCount <= 0) {
			ASSERT_FORMAT(false, "GetAvailableAssetCount Failed");
		}

		// Get the names of the assets contained in given asset library
		std::vector<HAPI_StringHandle> hapiAssetNames(assetCount);
		HAPIResultCode = FHoudiniApi::GetAvailableAssets(mSession.get(), libraryID, hapiAssetNames.data(), assetCount);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "GetAvailableAssets Failed");
		}

		// Convert HAPI_StringHandle to std::string
		std::vector<std::string> subAssetNames(assetCount);
		for (uint32_t i = 0; i < assetCount; i++) {
			int32_t stringBufferLength = 0u;
			HAPIResultCode = FHoudiniApi::GetStringBufLength(mSession.get(), hapiAssetNames[i], &stringBufferLength);
			if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
				ASSERT_FORMAT(false, "GetStringBufLength Failed");
			}
			subAssetNames[i].resize(stringBufferLength + 1);
			HAPIResultCode = FHoudiniApi::GetString(mSession.get(), hapiAssetNames[i], subAssetNames[i].data(), stringBufferLength);
			if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
				ASSERT_FORMAT(false, "GetString Failed");
			}
		}

		// Load and cook the HDA
		int32_t selectedSubassetIndex = 0;
		HAPI_NodeId newAssetID = -1;
		std::string topName = subAssetNames[selectedSubassetIndex];
		HAPIResultCode = FHoudiniApi::CreateNode(mSession.get(), -1, topName.c_str(), "", false, &newAssetID);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "CreateNode Failed");
		}

		// Waits until cooking has finished
		std::string assetName = "";
		FHoudiniApiUtility::ProcessHoudiniCookStatus(mSession.get(), assetName);

		// In case the cooking wasn't done previously, force it now.
		FHoudiniApiUtility::CookNodeInHoudini(mSession.get(), newAssetID, false, false, assetName);

		// Get the asset ID
		HAPI_AssetInfo assetInfo{};
		HAPIResultCode = FHoudiniApi::GetAssetInfo(mSession.get(), newAssetID, &assetInfo);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "GetAssetInfo Failed");
		}

		// Check for any errors
		HAPI_ErrorCodeBits errors{};
		FHoudiniApi::CheckForSpecificErrors(mSession.get(), newAssetID, HAPI_ERRORCODE_ASSET_DEF_NOT_FOUND, &errors);
		if (errors > 0) {
			std::cout << "Asset Missing Sub-asset Definitions" << std::endl;
			/*
				"There are undefined nodes. This is due to not being able to find specific " +
				"asset definitions. You might need to load other (dependent) HDAs first." 
				*/
		}

		// InternalSetAssetID(newAssetID);
		// session.RegisterAsset(this);

		HAPI_NodeInfo nodeInfo{};
		HAPIResultCode = FHoudiniApi::GetNodeInfo(mSession.get(), newAssetID, &nodeInfo);
		if (HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "GetNodeInfo Failed");
		}

		int32_t totalCookCount = 0;
		HAPIResultCode = FHoudiniApi::GetTotalCookCount(
			mSession.get(), 
			newAssetID,
			(int)(HAPI_NODETYPE_OBJ | HAPI_NODETYPE_SOP),
			(int)(HAPI_NODEFLAGS_OBJ_GEOMETRY | HAPI_NODEFLAGS_DISPLAY),
			true,
			&totalCookCount);

		// Create objects in this asset. It will create object nodes, geometry, and anything else required.

		// Fill in object infos and transforms based on node type and number of child objects
		std::vector<HAPI_ObjectInfo> objectInfos;
		std::vector<HAPI_Transform>  objectTransforms;
		FHoudiniApiUtility::GetObjectInfos(mSession.get(), newAssetID, nodeInfo, objectInfos, objectTransforms);
	}

}