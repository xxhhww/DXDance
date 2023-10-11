#include "OfflineTask/ResolveHDAFile.h"
#include "Tools/Assert.h"

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
		Houdini::HoudiniApiUtility::SetCookOptions(cookOptions);

		std::string HDASearchPath = ""; // = HEU_Platform.GetAllFoldersInPath(HEU_Defines.HEU_ENGINE_ASSETS + "/HDAs");
		std::string DSOSearchPath = ""; // = HEU_Platform.GetAllFoldersInPath(HEU_Defines.HEU_ENGINE_ASSETS + "/DSOs");
		std::string environmentFilePath = ""; // = HEU_Platform.GetHoudiniEngineEnvironmentFilePathFull();

		HAPIResultCode = FHoudiniApi::Initialize(mSession.get(), &cookOptions, true, -1, environmentFilePath.c_str(), HDASearchPath.c_str(), DSOSearchPath.c_str(), DSOSearchPath.c_str(), DSOSearchPath.c_str());
		if (HAPIResultCode != HAPI_RESULT_ALREADY_INITIALIZED && HAPIResultCode != HAPI_RESULT_SUCCESS) {
			ASSERT_FORMAT(false, "Initialize Failed");
		}

		mHoudiniAsset = std::make_unique<Houdini::HoudiniAsset>(mSession.get(), filepath, true, -1);
		mHoudiniAsset->Build(mSession.get());
	}

}