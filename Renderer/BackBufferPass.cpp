#include "BackBufferPass.h"
#include "RenderGraphBuilder.h"

namespace Renderer {

	void BackBufferPass::AddPass(RenderGraph& renderGraph) {
		renderGraph.AddPass(
			"BackBufferPass",
			[=](RenderGraphBuilder& builder) {
				builder.SetPassExecutionQueue(GHL::EGPUQueue::Graphics);

				builder.WriteRenderTarget("FinalOutput");
			},
			[=]() {
				int i = 32;

			});
	}

}