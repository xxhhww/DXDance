#pragma once
#include "RenderGraph.h"

namespace Renderer {

	template<typename ...Args>
	void RenderGraph::AddPass(Args&&... args) {
		mRenderGraphPasses.emplace_back(std::make_unique<RenderGraphPass>(std::forward<Args>(args)...));
		mPassNodes.emplace_back(std::make_unique<PassNode>());
		auto* passNode = mPassNodes.back();
		passNode->pass = mRenderGraphPasses.back();
	}


}