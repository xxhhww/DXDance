#include "RenderGraphBuilder.h"
#include "RenderGraphResource.h"
#include "RenderGraphResourceStorage.h"

#include "Tools/Assert.h"

namespace Renderer {

	RenderGraphBuilder::RenderGraphBuilder(RenderGraph::GraphNode* graphNode, RenderGraphResourceStorage* resourceStorage)
	: mGraphNode(graphNode)
	, mResourceStorage(resourceStorage) {}

	void RenderGraphBuilder::NewRenderTarget(const std::string& name, const NewTextureProperties& desc) {
		// 为节点添加信息
		mGraphNode->AddWriteDependency(name);
		mGraphNode->SetExpectedStates(name, GHL::EResourceState::RenderTarget);

		// 为资源添加信息
		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = desc;
		resource->SetInitialStates(GHL::EResourceState::RenderTarget);
		resource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::RenderTarget);
	}

	void RenderGraphBuilder::NewDepthStencil(const std::string& name, const NewTextureProperties& desc) {
		mGraphNode->AddWriteDependency(name);
		mGraphNode->SetExpectedStates(name, GHL::EResourceState::DepthWrite);

		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = desc;
		resource->SetInitialStates(GHL::EResourceState::DepthWrite);
		resource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::DepthWrite);
	}

	void RenderGraphBuilder::NewTexture(const std::string& name, const NewTextureProperties& desc) {
		mGraphNode->AddWriteDependency(name);
		mGraphNode->SetExpectedStates(name, GHL::EResourceState::UnorderedAccess);

		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = desc;
		resource->SetInitialStates(GHL::EResourceState::UnorderedAccess);
		resource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::UnorderedAccess);
	}

	void RenderGraphBuilder::NewBuffer(const std::string& name, const NewBufferProperties& desc) {
		mGraphNode->AddWriteDependency(name);
		mGraphNode->SetExpectedStates(name, GHL::EResourceState::UnorderedAccess);

		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = desc;
		resource->SetInitialStates(GHL::EResourceState::UnorderedAccess);
		resource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::UnorderedAccess);
	}

	void RenderGraphBuilder::ReadTexture(const std::string& name, const ShaderAccessFlag& accessFlag) {
		mGraphNode->AddReadDependency(name);
		GHL::EResourceState expectedStates;
		if (mGraphNode->executionQueueIndex == std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::General)) {
			// 由图形引擎访问
			switch (accessFlag) {
			case Renderer::ShaderAccessFlag::NonPixelShader:
				expectedStates = GHL::EResourceState::NonPixelShaderAccess;
				break;
			case Renderer::ShaderAccessFlag::PixelShader:
				expectedStates = GHL::EResourceState::PixelShaderAccess;
				break;
			case Renderer::ShaderAccessFlag::AnyShader:
				expectedStates = GHL::EResourceState::AnyShaderAccess;
				break;
			default:
				ASSERT_FORMAT(false, "Unsupported ShaderAccessFlag");
				break;
			}
		}
		else {
			// 由计算引擎访问
			expectedStates = GHL::EResourceState::NonPixelShaderAccess;
		}
		mGraphNode->SetExpectedStates(name, expectedStates);

		auto* resource = mResourceStorage->GetResource(name);
		resource->SetExpectedStates(mGraphNode->nodeIndex, expectedStates);
	}

	void RenderGraphBuilder::WriteTexture(const std::string& name) {
		mGraphNode->AddWriteDependency(name);
		mGraphNode->SetExpectedStates(name, GHL::EResourceState::UnorderedAccess);

		auto* resource = mResourceStorage->GetResource(name);
		resource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::UnorderedAccess);
	}

	void RenderGraphBuilder::ReadDepthStencil(const std::string& name) {
		mGraphNode->AddReadDependency(name);
		mGraphNode->SetExpectedStates(name, GHL::EResourceState::DepthRead);

		auto* resource = mResourceStorage->GetResource(name);
		resource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::DepthRead);
	}

	void RenderGraphBuilder::WriteDepthStencil(const std::string& name) {
		mGraphNode->AddWriteDependency(name);
		mGraphNode->SetExpectedStates(name, GHL::EResourceState::DepthWrite);

		auto* resource = mResourceStorage->GetResource(name);
		resource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::DepthWrite);
	}

	void RenderGraphBuilder::ReadBuffer(const std::string& name, const ShaderAccessFlag& accessFlag) {
		mGraphNode->AddReadDependency(name);
		GHL::EResourceState expectedStates;
		if (mGraphNode->executionQueueIndex == std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::General)) {
			// 由图形引擎访问
			switch (accessFlag) {
			case Renderer::ShaderAccessFlag::NonPixelShader:
				expectedStates = GHL::EResourceState::NonPixelShaderAccess;
				break;
			case Renderer::ShaderAccessFlag::PixelShader:
				expectedStates = GHL::EResourceState::PixelShaderAccess;
				break;
			case Renderer::ShaderAccessFlag::AnyShader:
				expectedStates = GHL::EResourceState::AnyShaderAccess;
				break;
			default:
				ASSERT_FORMAT(false, "Unsupported ShaderAccessFlag");
				break;
			}
		}
		else {
			// 由计算引擎访问
			expectedStates = GHL::EResourceState::NonPixelShaderAccess;
		}
		mGraphNode->SetExpectedStates(name, expectedStates);

		auto* resource = mResourceStorage->GetResource(name);
		resource->SetExpectedStates(mGraphNode->nodeIndex, expectedStates);
	}

	void RenderGraphBuilder::WriteBuffer(const std::string& name) {
		mGraphNode->AddWriteDependency(name);
		mGraphNode->SetExpectedStates(name, GHL::EResourceState::UnorderedAccess);

		auto* resource = mResourceStorage->GetResource(name);
		resource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::UnorderedAccess);
	}

	void RenderGraphBuilder::CopyTexture(const std::string& src, const std::string& dst) {
		mGraphNode->AddReadDependency(src);
		mGraphNode->AddWriteDependency(dst);
		mGraphNode->SetExpectedStates(src, GHL::EResourceState::CopySource);
		mGraphNode->SetExpectedStates(dst, GHL::EResourceState::CopyDestination);

		auto* srcResource = mResourceStorage->GetResource(src);
		srcResource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::CopySource);

		auto* dstResource = mResourceStorage->GetResource(dst);
		dstResource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::CopyDestination);
	}

	void RenderGraphBuilder::CopyBuffer(const std::string& src, const std::string dst) {
		mGraphNode->AddReadDependency(src);
		mGraphNode->AddWriteDependency(dst);
		mGraphNode->SetExpectedStates(src, GHL::EResourceState::CopySource);
		mGraphNode->SetExpectedStates(dst, GHL::EResourceState::CopyDestination);

		auto* srcResource = mResourceStorage->GetResource(src);
		srcResource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::CopySource);

		auto* dstResource = mResourceStorage->GetResource(dst);
		dstResource->SetExpectedStates(mGraphNode->nodeIndex, GHL::EResourceState::CopyDestination);
	}

	void RenderGraphBuilder::SetPassExecutionQueue(PassExecutionQueue queueIndex) {
		mGraphNode->SetExecutionQueue(queueIndex);
	}

}