#pragma once
#include "pbh.h"
#include "ShaderDesc.h"

namespace GHL {

	class Shader {
    public:
        using Blob = std::vector<uint8_t>;

	public:

        void SetDesc(const ShaderDesc& desc);

        void SetBytecode(void* data, size_t size);

        inline const auto& GetDesc()    const { return mDesc; }
        inline const void* GetPointer() const { return mBlob.data(); }
        inline const auto  GetLength()  const { return mBlob.size(); }

        inline D3D12_SHADER_BYTECODE D3DBytecode() const { 
            D3D12_SHADER_BYTECODE d3dBytecode{};
            d3dBytecode.pShaderBytecode = GetPointer();
            d3dBytecode.BytecodeLength = GetLength();
            return d3dBytecode;
        }

    private:
        ShaderDesc mDesc{};
        Blob mBlob;
	};

}