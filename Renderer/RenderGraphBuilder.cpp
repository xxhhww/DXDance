#include "RenderGraphBuilder.h"

namespace Renderer {

	RenderGraphBuilder::RenderGraphBuilder(RenderGraph::PassNode* passNode, RenderGraph* renderGraph) 
	: mPassNode(passNode)
	, mRenderGraph(renderGraph) {}

	void RenderGraphBuilder::NewRenderTarget(const std::string& name, const RGTextureDesc& desc) {

	}

	void RenderGraphBuilder::NewDepthStencil(const std::string& name, const RGTextureDesc& desc) {

	}

	void RenderGraphBuilder::NewTexture(const std::string& name, const RGTextureDesc& desc) {

	}

	void RenderGraphBuilder::NewBuffer(const std::string& name, const RGBufferDesc& desc) {

	}

	void RenderGraphBuilder::ReadTexture(const std::string& name) {

	}

	void RenderGraphBuilder::WriteTexture(const std::string& name) {

	}

	void RenderGraphBuilder::ReadDepthStencil(const std::string& name) {

	}

	void RenderGraphBuilder::WriteDepthStencil(const std::string& name) {

	}

	void RenderGraphBuilder::ReadBuffer(const std::string& name) {

	}

	void RenderGraphBuilder::WriteBuffer(const std::string& name) {

	}

	void RenderGraphBuilder::CopyTexture(const std::string& src, const std::string& dst) {

	}

	void RenderGraphBuilder::CopyBuffer(const std::string& src, const std::string dst) {

	}

	void RenderGraphBuilder::SetPassExecutionQueue(PassExecutionQueue queueIndex) {

	}

}