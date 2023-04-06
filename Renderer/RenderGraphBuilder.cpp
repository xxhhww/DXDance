#include "RenderGraphBuilder.h"
#include "RenderGraphResource.h"
#include "RenderGraphResourceStorage.h"

#include "Tools/Assert.h"

namespace Renderer {

	RenderGraphBuilder::RenderGraphBuilder(PassNode* passNode, RenderGraphResourceStorage* resourceStorage)
	: mPassNode(passNode)
	, mResourceStorage(resourceStorage) {}

	void RenderGraphBuilder::NewRenderTarget(const std::string& name, const NewTextureProperties& desc) {

		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = desc;
		resource->initialStates |= GHL::EResourceState::RenderTarget;
		for (uint32_t i = 0; i < desc.mipLevals; i++) {
			resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, i, GHL::EResourceState::RenderTarget);
		}

		mPassNode->AddWriteDependency(resource->resourceID, 0u, desc.mipLevals);
	}

	void RenderGraphBuilder::NewDepthStencil(const std::string& name, const NewTextureProperties& desc) {

		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = desc;
		resource->initialStates |= GHL::EResourceState::DepthWrite;
		for (uint32_t i = 0; i < desc.mipLevals; i++) {
			resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, i, GHL::EResourceState::DepthWrite);
		}

		mPassNode->AddWriteDependency(resource->resourceID, 0u, desc.mipLevals);

	}

	void RenderGraphBuilder::NewTexture(const std::string& name, const NewTextureProperties& desc) {

		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = desc;
		resource->initialStates |= GHL::EResourceState::UnorderedAccess;
		for (uint32_t i = 0; i < desc.mipLevals; i++) {
			resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, i, GHL::EResourceState::UnorderedAccess);
		}

		mPassNode->AddWriteDependency(resource->resourceID, 0u, desc.mipLevals);

	}

	void RenderGraphBuilder::NewBuffer(const std::string& name, const NewBufferProperties& desc) {

		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = desc;
		resource->initialStates |= GHL::EResourceState::UnorderedAccess;
		// Buffer Only Has a Subresource
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, 0, GHL::EResourceState::UnorderedAccess);

		mPassNode->AddWriteDependency(resource->resourceID, 0u, 1u);

	}

	void RenderGraphBuilder::ReadTexture(const std::string& name, const ShaderAccessFlag& accessFlag, uint32_t firstMip, uint32_t mipCount) {

		GHL::EResourceState expectedStates;
		if (mPassNode->executionQueueIndex == std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::General)) {
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

		auto* resource = mResourceStorage->GetResourceByName(name);

		ASSERT_FORMAT((firstMip + mipCount) > resource->resourceFormat.SubresourceCount(), "Out Of Subresource Range!");

		for (uint32_t i = 0; i < mipCount; i++) {
			resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, firstMip + i, expectedStates);
		}

		mPassNode->AddReadDependency(resource->resourceID, firstMip, mipCount);

	}

	void RenderGraphBuilder::WriteTexture(const std::string& name, uint32_t firstMip, uint32_t mipCount) {

		auto* resource = mResourceStorage->GetResourceByName(name);

		ASSERT_FORMAT((firstMip + mipCount) > resource->resourceFormat.SubresourceCount(), "Out Of Subresource Range!");

		for (uint32_t i = 0; i < mipCount; i++) {
			resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, firstMip + i, GHL::EResourceState::UnorderedAccess);
		}

		mPassNode->AddWriteDependency(resource->resourceID, firstMip, mipCount);

	}

	void RenderGraphBuilder::ReadDepthStencil(const std::string& name, uint32_t firstMip, uint32_t mipCount) {

		auto* resource = mResourceStorage->GetResourceByName(name);

		ASSERT_FORMAT((firstMip + mipCount) > resource->resourceFormat.SubresourceCount(), "Out Of Subresource Range!");

		for (uint32_t i = 0; i < mipCount; i++) {
			resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, firstMip + i, GHL::EResourceState::DepthRead);
		}

		mPassNode->AddReadDependency(resource->resourceID, firstMip, mipCount);

	}

	void RenderGraphBuilder::WriteDepthStencil(const std::string& name, uint32_t firstMip, uint32_t mipCount) {

		auto* resource = mResourceStorage->GetResourceByName(name);

		ASSERT_FORMAT((firstMip + mipCount) > resource->resourceFormat.SubresourceCount(), "Out Of Subresource Range!");

		for (uint32_t i = 0; i < mipCount; i++) {
			resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, firstMip + i, GHL::EResourceState::DepthWrite);
		}

		mPassNode->AddWriteDependency(resource->resourceID, firstMip, mipCount);

	}

	void RenderGraphBuilder::CopyTexture(const std::string& src, const std::string& dst) {

	}

	void RenderGraphBuilder::ReadBuffer(const std::string& name, const ShaderAccessFlag& accessFlag) {

		GHL::EResourceState expectedStates;
		if (mPassNode->executionQueueIndex == std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::General)) {
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

		auto* resource = mResourceStorage->GetResourceByName(name);
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, 0u, expectedStates);

		mPassNode->AddReadDependency(resource->resourceID, 0u, 1u);

	}

	void RenderGraphBuilder::WriteBuffer(const std::string& name) {

		auto* resource = mResourceStorage->GetResourceByName(name);
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, 0u, GHL::EResourceState::UnorderedAccess);

		mPassNode->AddWriteDependency(resource->resourceID, 0u, 1u);

	}

	void RenderGraphBuilder::CopyBuffer(const std::string& src, const std::string dst) {

	}

	void RenderGraphBuilder::SetPassExecutionQueue(PassExecutionQueue queueIndex) {
		mPassNode->SetExecutionQueue(queueIndex);
	}

}