#include "PipelineStateAllocator.h"

namespace Renderer {

    PipelineStateAllocator::PipelineStateAllocator(const GHL::Device* device, ShaderManger* shaderManger)
    : mDevice(device)
    , mShaderManger(shaderManger) {
        // TODO 创建一个"Signature_Root"的根签名


    }

    void PipelineStateAllocator::CreateGraphicsPSO(const std::string& name, const GraphicsStateConfigurator& configurator) {

    }

    void PipelineStateAllocator::CreateComputePSO(const std::string& name, const ComputeStateConfigurator& configurator) {

    }

    void PipelineStateAllocator::CreateRootSignature(const std::string& name, const RootSignatureConfigurator& configurator) {

    }

}