#pragma once
#include "RenderGraph.h"

namespace Renderer {

	template<typename ...Args>
	void RenderGraph::AddPass(Args&&... args) {
		mRenderGraphPasses.emplace_back(std::make_unique<RenderGraphPass>(std::forward<Args>(args)...));
		mPassNodes.emplace_back(std::make_unique<PassNode>());

		auto& renderPass = mRenderGraphPasses.back();
		auto& passNode = mPassNodes.back();
		passNode->renderPass = renderPass.get();
		passNode->passNodeIndex = mPassNodes.size() - 1u;
	}


}