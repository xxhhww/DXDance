#include "PipelineStateManger.h"
#include "ShaderManger.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace Renderer {

    PipelineStateManger::PipelineStateManger(const GHL::Device* device, ShaderManger* shaderManger)
    : mDevice(device)
    , mShaderManger(shaderManger) {
        // TODO 创建一个"Signature_Root"的根签名


    }

    void PipelineStateManger::CreateGraphicsPSO(const std::string& name, const GraphicsStateConfigurator& configurator) {
        ASSERT_FORMAT(GetD3DPipelineState(name) == nullptr, "Graphics Pipeline State: ", name.c_str(), " already exists.");

        GraphicsStateProxy proxy{};

        // 收集GraphicsState
        configurator(proxy);

        ASSERT_FORMAT(!proxy.vsFilename.empty(), "Vertex Shader Missing");
        ASSERT_FORMAT(!proxy.psFilename.empty(), "Pixel Shader Missing");

        std::unique_ptr<GHL::GraphicsPipelineState> pipelineState = std::make_unique<GHL::GraphicsPipelineState>(mDevice);

        pipelineState->desc.SampleMask = proxy.sampleMask;
        pipelineState->desc.SampleDesc = proxy.sampleDesc;
        pipelineState->desc.BlendState = proxy.blendDesc;
        pipelineState->desc.RasterizerState = proxy.rasterizerDesc;
        pipelineState->desc.DepthStencilState = proxy.depthStencilDesc;
        pipelineState->desc.DSVFormat = proxy.depthStencilFormat;
        pipelineState->desc.PrimitiveTopologyType = proxy.primitiveTopology;
        pipelineState->desc.NumRenderTargets = proxy.renderTargetFormatArray.size();
        for (uint32_t i = 0; i < pipelineState->desc.NumRenderTargets; i++) {
            pipelineState->desc.RTVFormats[i] = proxy.renderTargetFormatArray[i];
        }
        pipelineState->desc.NodeMask = mDevice->GetNodeMask();
        pipelineState->desc.InputLayout.pInputElementDescs = proxy.inputElementArray.data();
        pipelineState->desc.InputLayout.NumElements = proxy.inputElementArray.size();
        pipelineState->desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        pipelineState->desc.pRootSignature = GetD3DRootSignature(proxy.rootSignatureName);

        pipelineState->desc.VS = mShaderManger->GetShader(proxy.vsFilename, GHL::EShaderStage::VS)->D3DBytecode();
        pipelineState->desc.PS = mShaderManger->GetShader(proxy.psFilename, GHL::EShaderStage::PS)->D3DBytecode();

        if (proxy.gsFilename) {
            pipelineState->desc.GS = mShaderManger->GetShader(*proxy.gsFilename, GHL::EShaderStage::GS)->D3DBytecode();
        }

        pipelineState->Compile();
        pipelineState->SetDebugName(name);

        mPipelineStateMap[name] = std::move(pipelineState);

    }

    void PipelineStateManger::CreateComputePSO(const std::string& name, const ComputeStateConfigurator& configurator) {
        ASSERT_FORMAT(GetD3DPipelineState(name) == nullptr, "Compute Pipeline State: ", name.c_str(), " already exists.");

        ComputeStateProxy proxy{};

        // 收集GraphicsState
        configurator(proxy);

        ASSERT_FORMAT(!proxy.csFilename.empty(), "Compute Shader Missing");

        std::unique_ptr<GHL::ComputePipelineState> pipelineState = std::make_unique<GHL::ComputePipelineState>(mDevice);

        pipelineState->desc.CS = mShaderManger->GetShader(proxy.csFilename, GHL::EShaderStage::CS)->D3DBytecode();
        pipelineState->desc.NodeMask = mDevice->GetNodeMask();
        pipelineState->desc.Flags = D3D12_PIPELINE_STATE_FLAG_NONE;
        pipelineState->desc.pRootSignature = GetD3DRootSignature(proxy.rootSignatureName);

        pipelineState->Compile();
        pipelineState->SetDebugName(name);

        mPipelineStateMap[name] = std::move(pipelineState);

    }

    void PipelineStateManger::CreateRootSignature(const std::string& name, const RootSignatureConfigurator& configurator) {
        // TODO...
    }

}