#include "GBufferPass.h"
#include "RenderGraphBuilder.h"

namespace Renderer {

	void GBufferPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"GBufferPass",
			[=](RenderGraphBuilder& builder) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				NewTextureProperties _GBufferProperties;
				_GBufferProperties.width  = 1920u;
				_GBufferProperties.height = 1080u;
				_GBufferProperties.format = DXGI_FORMAT_R8G8B8A8_UNORM;
				
				builder.DeclareTexture("GBufferBaseColor", _GBufferProperties);
				builder.DeclareTexture("GBufferPosition", _GBufferProperties);
				builder.DeclareTexture("GBufferNormal", _GBufferProperties);
				builder.DeclareTexture("GBufferMixed", _GBufferProperties);

				_GBufferProperties.format = DXGI_FORMAT_D24_UNORM_S8_UINT;

				builder.DeclareTexture("GBufferDepth", _GBufferProperties);

				builder.WriteRenderTarget("GBufferBaseColor");
				builder.WriteRenderTarget("GBufferPosition");
				builder.WriteRenderTarget("GBufferNormal");
				builder.WriteRenderTarget("GBufferMixed");
				builder.WriteDepthStencil("GBufferDepth");
			},
			[=]() {


			});

	}

}