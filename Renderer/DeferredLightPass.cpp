#include "DeferredLightPass.h"
#include "RenderGraphBuilder.h"

namespace Renderer {

	void DeferredLightPass::AddPass(RenderGraph& renderGraph) {

		renderGraph.AddPass(
			"DeferredLightPass",
			[=](RenderGraphBuilder& builder) {


			},
			[=]() {


			});

	}

}