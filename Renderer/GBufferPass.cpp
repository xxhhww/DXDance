#include "Renderer/GBufferPass.h"
#include "Renderer/RenderGraphBuilder.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/ShaderManger.h"


namespace Renderer {

	void GBufferPass::AddPass(RenderGraph& renderGraph) {

		auto& finalOutputDesc = 
			renderGraph.GetPipelineResourceStorage()->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();

		renderGraph.AddPass(
			"GBufferPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				NewTextureProperties _GBufferAlbedoMetalnessProperties{};
				_GBufferAlbedoMetalnessProperties.width = finalOutputDesc.width;
				_GBufferAlbedoMetalnessProperties.height = finalOutputDesc.height;
				_GBufferAlbedoMetalnessProperties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				_GBufferAlbedoMetalnessProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("GBufferAlbedoMetalness", _GBufferAlbedoMetalnessProperties);
				builder.WriteRenderTarget("GBufferAlbedoMetalness");

				NewTextureProperties _GBufferPositionEmissionProperties{};
				_GBufferPositionEmissionProperties.width  = finalOutputDesc.width;
				_GBufferPositionEmissionProperties.height = finalOutputDesc.height;
				_GBufferPositionEmissionProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_GBufferPositionEmissionProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("GBufferPositionEmission", _GBufferPositionEmissionProperties);
				builder.WriteRenderTarget("GBufferPositionEmission");

				NewTextureProperties _GBufferNormalRoughnessProperties{};
				_GBufferNormalRoughnessProperties.width = finalOutputDesc.width;
				_GBufferNormalRoughnessProperties.height = finalOutputDesc.height;
				_GBufferNormalRoughnessProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_GBufferNormalRoughnessProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("GBufferNormalRoughness", _GBufferNormalRoughnessProperties);
				builder.WriteRenderTarget("GBufferNormalRoughness");

				NewTextureProperties _GBufferMotionVectorProperties{};
				_GBufferMotionVectorProperties.width = finalOutputDesc.width;
				_GBufferMotionVectorProperties.height = finalOutputDesc.height;
				_GBufferMotionVectorProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_GBufferMotionVectorProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("GBufferMotionVector", _GBufferMotionVectorProperties);
				builder.WriteRenderTarget("GBufferMotionVector");

				NewTextureProperties _GBufferViewDepthProperties{};
				_GBufferViewDepthProperties.width = finalOutputDesc.width;
				_GBufferViewDepthProperties.height = finalOutputDesc.height;
				_GBufferViewDepthProperties.format = DXGI_FORMAT_R32_FLOAT;
				_GBufferViewDepthProperties.clearValue = GHL::ColorClearValue{ std::numeric_limits<float>::max() };
				builder.DeclareTexture("GBufferViewDepth", _GBufferViewDepthProperties);
				builder.WriteRenderTarget("GBufferViewDepth");

				NewTextureProperties _GBufferDepthStencilProperties{};
				_GBufferDepthStencilProperties.width = finalOutputDesc.width;
				_GBufferDepthStencilProperties.height = finalOutputDesc.height;
				_GBufferDepthStencilProperties.format = DXGI_FORMAT_D32_FLOAT;
				_GBufferDepthStencilProperties.clearValue = GHL::DepthStencilClearValue{ 1.0f, 0u };
				builder.DeclareTexture("GBufferDepthStencil", _GBufferDepthStencilProperties);
				builder.WriteDepthStencil("GBufferDepthStencil");

				shaderManger.CreateGraphicsShader("GBufferPass",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/StandardGBufferPass.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
						proxy.renderTargetFormatArray = {
							DXGI_FORMAT_R8G8B8A8_UNORM,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R32_FLOAT
						};
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* gBufferAlbedoMetalness  = resourceStorage->GetResourceByName("GBufferAlbedoMetalness")->GetTexture();
				auto* gBufferPositionEmission = resourceStorage->GetResourceByName("GBufferPositionEmission")->GetTexture();
				auto* gBufferNormalRoughness  = resourceStorage->GetResourceByName("GBufferNormalRoughness")->GetTexture();
				auto* gBufferMotionVector     = resourceStorage->GetResourceByName("GBufferMotionVector")->GetTexture();
				auto* gBufferViewDepth        = resourceStorage->GetResourceByName("GBufferViewDepth")->GetTexture();
				auto* gBufferDepthStencil     = resourceStorage->GetResourceByName("GBufferDepthStencil")->GetTexture();

				commandBuffer.ClearRenderTarget(gBufferAlbedoMetalness);
				commandBuffer.ClearRenderTarget(gBufferPositionEmission);
				commandBuffer.ClearRenderTarget(gBufferNormalRoughness);
				commandBuffer.ClearRenderTarget(gBufferMotionVector);
				commandBuffer.ClearRenderTarget(gBufferViewDepth);
				commandBuffer.ClearDepth(gBufferDepthStencil, 1.0f);
			});
	}

}