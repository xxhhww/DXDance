#pragma once
#include "Shader.h"
#include <filesystem>

namespace GHL {

    struct ShaderCompilationResult {
        Shader compiledShader;
        std::vector<std::string> includes;
        uint64_t shaderHash[2];
    };

	class ShaderCompiler {
    public:
        ShaderCompiler();
        ~ShaderCompiler() = default;

        ShaderCompilationResult CompileShader(const ShaderDesc& desc) const;

    private:

        Microsoft::WRL::ComPtr<IDxcLibrary> mLibrary;
        Microsoft::WRL::ComPtr<IDxcCompiler3> mCompiler;
	};
}