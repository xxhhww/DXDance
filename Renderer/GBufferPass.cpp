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

				NewTextureProperties _GBufferProperties;
				_GBufferProperties.width  = finalOutputDesc.width;
				_GBufferProperties.height = finalOutputDesc.height;
				_GBufferProperties.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
				_GBufferProperties.clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
				builder.DeclareTexture("GBufferAlbedo", _GBufferProperties);
				builder.DeclareTexture("GBufferPosition", _GBufferProperties);
				builder.DeclareTexture("GBufferNormal", _GBufferProperties);
				builder.WriteRenderTarget("GBufferAlbedo");
				builder.WriteRenderTarget("GBufferPosition");
				builder.WriteRenderTarget("GBufferNormal");

				_GBufferProperties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				builder.DeclareTexture("GBufferMRE", _GBufferProperties);
				builder.WriteRenderTarget("GBufferMRE");

				_GBufferProperties.format = DXGI_FORMAT_D24_UNORM_S8_UINT;
				_GBufferProperties.clearValue = GHL::DepthStencilClearValue{ 1.0f, 0u };
				builder.DeclareTexture("GBufferDepth", _GBufferProperties);
				builder.WriteDepthStencil("GBufferDepth");

				shaderManger.CreateGraphicsShader("GBufferPass",
					[](GraphicsStateProxy& proxy) {
						proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/StandardGBufferPass.hlsl";
						proxy.psFilepath = proxy.vsFilepath;
						proxy.depthStencilDesc.DepthEnable = true;
						proxy.depthStencilFormat = DXGI_FORMAT_D24_UNORM_S8_UINT;
						proxy.renderTargetFormatArray = { 
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R16G16B16A16_FLOAT,
							DXGI_FORMAT_R8G8B8A8_UNORM
						};
					});
			},
			[=](CommandBuffer& commandBuffer, RenderContext& renderContext) {
				auto* dynamicAllocator = renderContext.dynamicAllocator;
				auto* resourceStorage = renderContext.resourceStorage;
				auto* commandSignatureManger = renderContext.commandSignatureManger;

				auto* gbufferAlbedo = resourceStorage->GetResourceByName("GBufferAlbedo")->GetTexture();
				auto* gbufferPosition = resourceStorage->GetResourceByName("GBufferPosition")->GetTexture();
				auto* gbufferNormal = resourceStorage->GetResourceByName("GBufferNormal")->GetTexture();
				auto* gbufferMRE = resourceStorage->GetResourceByName("GBufferMRE")->GetTexture();
				auto* gbufferDepth = resourceStorage->GetResourceByName("GBufferDepth")->GetTexture();

				commandBuffer.ClearRenderTarget(gbufferAlbedo);
				commandBuffer.ClearRenderTarget(gbufferPosition);
				commandBuffer.ClearRenderTarget(gbufferNormal);
				commandBuffer.ClearRenderTarget(gbufferMRE);
				commandBuffer.ClearDepth(gbufferDepth, 1.0f);
			});
	}

}