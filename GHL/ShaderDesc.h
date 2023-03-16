#pragma once
#include "pbh.h"
#include <string>
#include <vector>

namespace GHL {

	struct ShaderMacro {
		std::string name;
		std::string value;
	};

	enum class EShaderStage {
		VS,
		PS,
		HS,
		DS,
		GS,
		CS,
		MS,
		AS,
	};

	enum class EShaderModel {
		SM_6_0,
		SM_6_1,
		SM_6_2,
		SM_6_3,
		SM_6_4,
		SM_6_5,
		SM_6_6
	};

	struct ShaderDesc {
		EShaderStage stage = EShaderStage::VS;
		EShaderModel model = EShaderModel::SM_6_6;
		std::string path{ "" };
		std::string entryPoint{ "" };
		std::vector<ShaderMacro> macros{};
		bool debugBuild{ true };
		bool separatePDB{ false };
	};

	std::string GenProfileString(EShaderStage stage, EShaderModel model);
}