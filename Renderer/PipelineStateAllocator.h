#pragma once
#include "GHL/PipelineState.h"
#include "GHL/RootSignature.h"

#include <string>
#include <optional>
#include <vector>
#include <functional>
#include <map>
#include <memory>

namespace Renderer {

    class GraphicsStateProxy {
    public:

        GraphicsStateProxy() {
            // 部分初始化
            sampleMask = UINT_MAX;
            blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
            sampleDesc.Count = 1u;
            sampleDesc.Quality = 0u;
            rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
            depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
            depthStencilFormat = DXGI_FORMAT_UNKNOWN;
            primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
            renderTargetFormatArray = { DXGI_FORMAT_R8G8B8A8_UNORM };
            inputElementArray = {
                { "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
                { "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                { "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
                { "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
            };

            rootSignatureName = "Signature_Root";
        }

        ~GraphicsStateProxy() = default;

        std::string vsFilename;
        std::string psFilename;
        std::optional<std::string> gsFilename;

        UINT sampleMask;
        D3D12_BLEND_DESC blendDesc;
        DXGI_SAMPLE_DESC sampleDesc;
        D3D12_RASTERIZER_DESC rasterizerDesc;
        D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
        DXGI_FORMAT depthStencilFormat;
        D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology;
        std::vector<DXGI_FORMAT> renderTargetFormatArray;
        std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementArray;

        std::string rootSignatureName;
    };

    class ComputeStateProxy {
    public:

        ComputeStateProxy() {

            rootSignatureName = "Signature_Root";
        }

        ~ComputeStateProxy() = default;

        std::string csFilename;

        std::string rootSignatureName;
    };

    class RootSignatureProxy {
    public:

    };


    class ShaderManger;

    class PipelineStateAllocator {
    public:
        using GraphicsStateConfigurator = std::function<void(GraphicsStateProxy&)>;
        using ComputeStateConfigurator  = std::function<void(ComputeStateProxy&)>;
        using RootSignatureConfigurator = std::function<void(RootSignatureProxy&)>;

        PipelineStateAllocator(const GHL::Device* device, ShaderManger* shaderManger);

        void CreateGraphicsPSO(const std::string& name, const GraphicsStateConfigurator& configurator);
        
        void CreateComputePSO(const std::string& name, const ComputeStateConfigurator& configurator);

        void CreateRootSignature(const std::string& name, const RootSignatureConfigurator& configurator);
        
        inline const auto* GetD3DPipelineState(const std::string& name) const { return mPipelineStateMap.at(name).get()->D3DPipelineState(); }
        inline const auto* GetD3DRootSignature(const std::string& name) const { return mRootSignatureMap.at(name).get()->D3DRootSignature(); }

    private:
        const GHL::Device* mDevice{ nullptr };
        ShaderManger* mShaderManger{ nullptr };

        std::unordered_map<std::string, std::unique_ptr<GHL::PipelineState>> mPipelineStateMap;
        std::unordered_map<std::string, std::unique_ptr<GHL::RootSignature>> mRootSignatureMap;

    };

}