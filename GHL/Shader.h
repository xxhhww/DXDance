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

        inline const auto* GetBlob() const { return mBlob.Get(); }
        inline const auto* GetPDBBlob() const { return mPDBBlob.Get(); }
        inline const auto& GetBinary() const { return mBinary; }
        inline const auto& GetPDBBinary() const { return mPDBBinary; }

        inline D3D12_SHADER_BYTECODE D3DBytecode() const { return { mBlob->GetBufferPointer(), mBlob->GetBufferSize() }; }

    private:
        std::string  mEntryPoint;
        EShaderStage mStage;
        Microsoft::WRL::ComPtr<IDxcBlob> mBlob;
        Microsoft::WRL::ComPtr<IDxcBlob> mPDBBlob;
        CompiledBinary mBinary;
        CompiledBinary mPDBBinary;

	};

}