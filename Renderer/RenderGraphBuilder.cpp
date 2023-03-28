#include "RenderGraphBuilder.h"

namespace Renderer {

	RenderGraphBuilder::RenderGraphBuilder(RenderGraph::GraphNode* graphNode, RenderGraph* renderGraph) 
	: mGraphNode(graphNode)
	, mRenderGraph(renderGraph) {}

	void RenderGraphBuilder::NewRenderTarget(const std::string& name, const RGTextureDesc& desc) {
		mGraphNode->AddWriteDependency(name);
	}

	void RenderGraphBuilder::NewDepthStencil(const std::string& name, const RGTextureDesc& desc) {
		mGraphNode->AddWriteDependency(name);
	}

	void RenderGraphBuilder::NewTexture(const std::string& name, const RGTextureDesc& desc) {
		mGraphNode->AddWriteDependency(name);
	}

	void RenderGraphBuilder::NewBuffer(const std::string& name, const RGBufferDesc& desc) {
		mGraphNode->AddWriteDependency(name);
	}

	void RenderGraphBuilder::ReadTexture(const std::string& name) {
		mGraphNode->AddReadDependency(name);
	}

	void RenderGraphBuilder::WriteTexture(const std::string& name) {
		mGraphNode->AddWriteDependency(name);
	}

	void RenderGraphBuilder::ReadDepthStencil(const std::string& name) {
		mGraphNode->AddReadDependency(name);
	}

	void RenderGraphBuilder::WriteDepthStencil(const std::string& name) {
		mGraphNode->AddWriteDependency(name);
	}

	void RenderGraphBuilder::ReadBuffer(const std::string& name) {
		mGraphNode->AddReadDependency(name);
	}

	void RenderGraphBuilder::WriteBuffer(const std::string& name) {
		mGraphNode->AddWriteDependency(name);
	}

	void RenderGraphBuilder::CopyTexture(const std::string& src, const std::string& dst) {
		mGraphNode->AddReadDependency(src);
		mGraphNode->AddWriteDependency(dst);
	}

	void RenderGraphBuilder::CopyBuffer(const std::string& src, const std::string dst) {
		mGraphNode->AddReadDependency(src);
		mGraphNode->AddWriteDependency(dst);
	}

	void RenderGraphBuilder::SetPassExecutionQueue(PassExecutionQueue queueIndex) {
		mGraphNode->SetExecutionQueue(queueIndex);
	}

}