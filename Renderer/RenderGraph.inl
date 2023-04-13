#pragma once
#include "RenderGraph.h"

namespace Renderer {

	template<typename ...Args>
	void RenderGraph::AddPass(Args&&... args) {
		mRenderGraphPasses.emplace_back(std::make_unique<RenderGraphPass>(std::forward<Args>(args)...));
		mGraphNodes.emplace_back(std::make_unique<PassNode>());
		mPassNodes.emplace_back(static_cast<PassNode*>(mGraphNodes.back().get()));

		auto* passNode = mPassNodes.back();
		passNode->graphNodeIndex = mGraphNodes.size() - 1u;
		passNode->passNodeIndex = mPassNodes.size() - 1u;
		passNode->renderPass = mRenderGraphPasses.back().get();
	}


}