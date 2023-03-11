#pragma once
#include "pbh.h"
#include <string>
#include <vector>

namespace GHL {

	struct ShaderMacro {
		std::wstring name;
		std::wstring value;
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
		Count
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

	enum class EShaderCompilerFlag {
		ShaderCompilerFlag_None = 0,
		ShaderCompilerFlag_Debug = 1 << 0,
		ShaderCompilerFlag_DisableOptimization = 1 << 1
	};

	struct ShaderDesc {
		EShaderStage stage = EShaderStage::Count;
		EShaderModel model = EShaderModel::SM_6_6;
		std::string file{ "" };
		std::string entryPoint{ "" };
		std::vector<ShaderMacro> macros{};
		EShaderCompilerFlag flags = EShaderCompilerFlag::ShaderCompilerFlag_None;
	};

}