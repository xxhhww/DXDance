#include "GBufferPass.h"
#include "RenderGraphBuilder.h"

namespace Renderer {

	void GBufferPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"GBufferPass",
			[=](RenderGraphBuilder& builder) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				NewTextureProperties properties;
				properties.width  = 1920u;
				properties.height = 1080u;
				properties.format = DXGI_FORMAT_R16G16B16A16_UNORM;
				
				builder.NewRenderTarget("GBufferBaseColor", properties);
				builder.NewRenderTarget("GBufferPosition", properties);
				builder.NewRenderTarget("GBufferNormal", properties);

				properties.format = DXGI_FORMAT_R8G8B8A8_UNORM;

				builder.NewRenderTarget("GBufferMixed", properties);

				properties.format = DXGI_FORMAT_D24_UNORM_S8_UINT;

				builder.NewDepthStencil("GBufferDepth", properties);
			},
			[=]() {


			});

	}

}