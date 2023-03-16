#include "Shader.h"

namespace GHL {
	
    Shader::Shader(const Microsoft::WRL::ComPtr<IDxcBlob>& blob, const Microsoft::WRL::ComPtr<IDxcBlob>& pdbBlob, const std::string& entryPoint, EShaderStage stage)
    : mBlob{ blob }
    , mPDBBlob{ pdbBlob }
    , mBinary{ blob != nullptr ? CompiledBinary{(uint8_t*)blob->GetBufferPointer(), blob->GetBufferSize()} : CompiledBinary{nullptr, 0} }
    , mPDBBinary{ pdbBlob != nullptr ? CompiledBinary{(uint8_t*)pdbBlob->GetBufferPointer(), pdbBlob->GetBufferSize()} : CompiledBinary{nullptr, 0} }
    , mEntryPoint{ entryPoint }
    , mStage{ stage } {}

}