#include "DeferredLightPass.h"
#include "RenderGraphBuilder.h"

namespace Renderer {

	void DeferredLightPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"DeferredLightPass",
			[=](RenderGraphBuilder& builder) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Compute);

				builder.ReadTexture("GBufferBaseColor", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferPosition", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferNormal", ShaderAccessFlag::NonPixelShader);
				builder.ReadTexture("GBufferMixed", ShaderAccessFlag::NonPixelShader);

				NewTextureProperties properties;
				properties.width = 1920u;
				properties.height = 1080u;
				properties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				builder.NewTexture("DeferredLightOutput", properties);
			},
			[=]() {


			});

	}

}