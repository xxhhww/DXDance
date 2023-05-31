#include "DeferredLightPass.h"
#include "RenderGraphBuilder.h"

namespace Renderer {

	void DeferredLightPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"DeferredLightPass",
			[=](RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				NewTextureProperties _DeferredLightOutputProperties;
				_DeferredLightOutputProperties.width = 1920u;
				_DeferredLightOutputProperties.height = 1080u;
				_DeferredLightOutputProperties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				builder.DeclareTexture("DeferredLightOutput", _DeferredLightOutputProperties);
				builder.WriteTexture("DeferredLightOutput");

				builder.ReadTexture("GBufferBaseColor", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferPosition", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferNormal", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferMixed", ShaderAccessFlag::NonPixelShader);
			},
			[=](CommandListWrap& commandList, RenderContext& context) {


			});

	}

}