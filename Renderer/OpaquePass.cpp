#include "Renderer/OpaquePass.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/ShaderManger.h"

namespace Renderer {

	void OpaquePass::AddPreDepthPass(RenderGraph& renderGraph) {
	}

	void OpaquePass::AddShadowPass(RenderGraph& renderGraph) {
	}

	void OpaquePass::AddForwardPlusPass(RenderGraph& renderGraph) {
		auto& finalOutputDesc =
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass(
			"OpaquePass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				NewTextureProperties _ShadingResultProperties{};
				_ShadingResultProperties.width = finalOutputDesc.width;
				_ShadingResultProperties.height = finalOutputDesc.height;
				_ShadingResultProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_ShadingResultProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("ShadingResult", _ShadingResultProperties);
				builder.WriteRenderTarget("ShadingResult");

				NewTextureProperties _NormalRoughnessProperties{};
				_NormalRoughnessProperties.width = finalOutputDesc.width;
				_NormalRoughnessProperties.height = finalOutputDesc.height;
				_NormalRoughnessProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_NormalRoughnessProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("NormalRoughness", _NormalRoughnessProperties);
				builder.WriteRenderTarget("NormalRoughness");

				NewTextureProperties _ScreenVelocityProperties{};
				_ScreenVelocityProperties.width = finalOutputDesc.width;
				_ScreenVelocityProperties.height = finalOutputDesc.height;
				_ScreenVelocityProperties.format = DXGI_FORMAT_R16G16_FLOAT;
				_ScreenVelocityProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("ScreenVelocity", _ScreenVelocityProperties);
				builder.WriteRenderTarget("ScreenVelocity");

				NewTextureProperties _DepthStencilProperties{};
				_DepthStencilProperties.width = finalOutputDesc.width;
				_DepthStencilProperties.height = finalOutputDesc.height;
				_DepthStencilProperties.format = DXGI_FORMAT_D32_FLOAT;
				_DepthStencilProperties.clearValue = GHL::DepthStencilClearValue{ 1.0f, 0u };
				builder.DeclareTexture("DepthStencil", _DepthStencilProperties);
				builder.WriteDepthStencil("DepthStencil");

				shaderManger.CreateGraphicsShader("OpaquePass",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/Opaque/OpaquePass.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						proxy.renderTargetFormatArray = {
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16_FLOAT,
						};
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;

				auto* shadingResult   = resourceStorage->GetResourceByName("ShadingResult")->GetTexture();
				auto* normalRoughness = resourceStorage->GetResourceByName("NormalRoughness")->GetTexture();
				auto* screenVelocity  = resourceStorage->GetResourceByName("ScreenVelocity")->GetTexture();
				auto* depthStencil    = resourceStorage->GetResourceByName("DepthStencil")->GetTexture();

				commandBuffer.ClearRenderTarget(shadingResult);
				commandBuffer.ClearRenderTarget(normalRoughness);
				commandBuffer.ClearRenderTarget(screenVelocity);
				commandBuffer.ClearDepth(depthStencil, 1.0f);
			});
	}

	void OpaquePass::AddGBufferPass(RenderGraph& renderGraph) {

	}

}