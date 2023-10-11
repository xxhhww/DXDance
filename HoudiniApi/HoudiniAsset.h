#pragma once
#include "HoudiniApi/HoudiniApi.h"
#include "HoudiniApi/HoudiniObjectNode.h"
#include <string>
#include <vector>

namespace Houdini {

	struct HoudiniAsset {
	public:
		std::string    assetName;
		std::string    assetOpName;
		std::string    assetHelp;
		HAPI_AssetInfo assetInfo;
		HAPI_NodeId    assetID;
		HAPI_NodeInfo  nodeInfo;

		int32_t selectedSubassetIndex{ 0 };

		bool useOutputNodes{ false };
		bool ignoreNonDisplayNodes{ false };
		
		int32_t totalCookCount;

		std::vector<std::string> subAssetNames;
		std::vector<std::unique_ptr<HoudiniObjectNode>> objectNodes;

	public:
		HoudiniAsset(const HAPI_Session* session, const std::string& hdaFilepath, bool bPromptForSubasset, int32_t desiredSubassetIndex);

		void Build(const HAPI_Session* session);

		void ProcessObjectNode(const HAPI_Session* session);

		void GenerateGeometry(const HAPI_Session* session, bool bRebuild = true);

		void GeneratePartInstances(const HAPI_Session* session);
	};

}