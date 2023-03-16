#pragma once
#include "pbh.h"
#include "ShaderDesc.h"

namespace GHL {

	struct CompiledBinary {
		uint8_t* Data = nullptr;
		uint64_t Size = 0;
	};

	class Shader {
	public:
        Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const Microsoft::WRL::ComPtr<IDxcBlob>& pdbBlob, const std::string& entryPoint, EShaderStage stage);
        ~Shader() = default;

        /*
        * Get·½·¨
        */


    private:
        std::string  mEntryPoint;
        EShaderStage mStage;
        Microsoft::WRL::ComPtr<IDxcBlob> mBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> mPDBBlob;
        CompiledBinary mBinary;
        CompiledBinary mPDBBinary;
	};

}