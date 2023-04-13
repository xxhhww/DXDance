#include "GBufferPass.h"
#include "RenderGraphBuilder.h"

namespace Renderer {

	void GBufferPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"GBufferPass",
			[=](RenderGraphBuilder& builder) {


			},
			[=]() {


			});

	}

}