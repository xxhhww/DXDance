#pragma once
#include "RenderGraph.h"

namespace Renderer {

	template<typename ...Args>
	void RenderGraph::AddPass(Args&&... args) {
		mRenderGraphPasses.emplace_back(std::make_unique<RenderGraphPass>(std::forward<Args>(args)...));
		mGraphNodes.emplace_back(std::make_unique<GraphNode>());

		auto& renderPass = mRenderGraphPasses.back();
		auto& graphNode = mGraphNodes.back();
		graphNode->pass = renderPass.get();
		graphNode->nodeIndex = mGraphNodes.size() - 1u;
	}


}