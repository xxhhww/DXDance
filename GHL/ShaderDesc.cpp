#include "ShaderDesc.h"
#include "Tools/Assert.h"

namespace GHL {

	std::string GenProfileString(EShaderStage stage, EShaderModel model) {
		std::string profileString;

		switch (stage) {
		case EShaderStage::VS: profileString = "vs_"; break;
		case EShaderStage::HS: profileString = "hs_"; break;
		case EShaderStage::DS: profileString = "ds_"; break;
		case EShaderStage::GS: profileString = "gs_"; break;
		case EShaderStage::PS: profileString = "ps_"; break;
		case EShaderStage::CS: profileString = "cs_"; break;
		default:
			ASSERT_FORMAT(false, "Unsupported Shader Stage");
			break;
		}

		switch (model) {
		case EShaderModel::SM_6_3: profileString += "6_3"; break;
		case EShaderModel::SM_6_4: profileString += "6_4"; break;
		case EShaderModel::SM_6_5: profileString += "6_5"; break;
		case EShaderModel::SM_6_6: profileString += "6_6"; break;
		default:
			ASSERT_FORMAT(false, "Unsupported Shader Model");
			break;
		}

		return profileString;
	}

}