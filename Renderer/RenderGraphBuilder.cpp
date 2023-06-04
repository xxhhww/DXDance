#include "RenderGraphBuilder.h"
#include "RenderGraphResource.h"
#include "RenderGraphResourceStorage.h"

#include "Tools/Assert.h"

namespace Renderer {

	RenderGraphBuilder::RenderGraphBuilder(PassNode* passNode, RenderGraphResourceStorage* resourceStorage)
	: mPassNode(passNode)
	, mResourceStorage(resourceStorage) {}

	RenderGraphResourceID RenderGraphBuilder::DeclareTexture(const std::string& name, const NewTextureProperties& properties) {
		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = properties;
		return resource->resourceID;
	}

	RenderGraphResourceID RenderGraphBuilder::DeclareBuffer(const std::string& name, const NewBufferProperties& properties) {
		auto* resource = mResourceStorage->DeclareResource(name);
		resource->newResourceProperties = properties;
		return resource->resourceID;
	}

	RenderGraphResourceID RenderGraphBuilder::WriteRenderTarget(const std::string& name, uint32_t mipLevel) {
		auto* resource = mResourceStorage->GetResourceByName(name);
		ASSERT_FORMAT(mipLevel < resource->resourceFormat.SubresourceCount(), "Out Of Subresource Range!");
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, mipLevel, GHL::EResourceState::RenderTarget);

		mPassNode->AddWriteDependency(resource->resourceID, mipLevel, 1u, false);

		return resource->resourceID;
	}

	RenderGraphResourceID RenderGraphBuilder::WriteDepthStencil(const std::string& name, uint32_t mipLevel) {
		auto* resource = mResourceStorage->GetResourceByName(name);
		ASSERT_FORMAT(mipLevel < resource->resourceFormat.SubresourceCount(), "Out Of Subresource Range!");
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, mipLevel, GHL::EResourceState::DepthWrite);

		mPassNode->AddWriteDependency(resource->resourceID, mipLevel, 1u, false);

		return resource->resourceID;
	}

	RenderGraphResourceID RenderGraphBuilder::WriteTexture(const std::string& name, uint32_t mipLevel) {

		auto* resource = mResourceStorage->GetResourceByName(name);
		ASSERT_FORMAT(mipLevel < resource->resourceFormat.SubresourceCount(), "Out Of Subresource Range!");
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, mipLevel, GHL::EResourceState::UnorderedAccess);

		mPassNode->AddWriteDependency(resource->resourceID, mipLevel, 1u, false);

		return resource->resourceID;

	}

	RenderGraphResourceID RenderGraphBuilder::ReadTexture(const std::string& name, const ShaderAccessFlag& accessFlag, uint32_t mipLevel) {

		GHL::EResourceState expectedStates;
		if (mPassNode->executionQueueIndex == std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Graphics)) {
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
		ASSERT_FORMAT(mipLevel < resource->resourceFormat.SubresourceCount(), "Out Of Subresource Range!");
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, mipLevel, expectedStates);

		mPassNode->AddReadDependency(resource->resourceID, mipLevel, 1u, false);

		return resource->resourceID;
	}

	RenderGraphResourceID RenderGraphBuilder::ReadDepthStencil(const std::string& name, uint32_t mipLevel) {
		auto* resource = mResourceStorage->GetResourceByName(name);
		ASSERT_FORMAT(mipLevel < resource->resourceFormat.SubresourceCount(), "Out Of Subresource Range!");
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, mipLevel, GHL::EResourceState::DepthRead);

		mPassNode->AddReadDependency(resource->resourceID, mipLevel, 1u, false);

		return resource->resourceID;
	}

	RenderGraphResourceID RenderGraphBuilder::WriteBuffer(const std::string& name) {
		auto* resource = mResourceStorage->GetResourceByName(name);
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, 0, GHL::EResourceState::UnorderedAccess);
		mPassNode->AddWriteDependency(resource->resourceID, 0u, 1u, true);

		return resource->resourceID;
	}

	RenderGraphResourceID RenderGraphBuilder::ReadBuffer(const std::string& name, const ShaderAccessFlag& accessFlag) {

		GHL::EResourceState expectedStates;
		if (mPassNode->executionQueueIndex == std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Graphics)) {
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

		mPassNode->AddReadDependency(resource->resourceID, 0u, 1u, true);

		return resource->resourceID;
	}

	RenderGraphResourceID RenderGraphBuilder::WriteCopyDstBuffer(const std::string& name) {
		auto* resource = mResourceStorage->GetResourceByName(name);
		resource->SetSubresourceRequestedInfo(mPassNode->passNodeIndex, 0, GHL::EResourceState::CopyDestination);
		mPassNode->AddWriteDependency(resource->resourceID, 0u, 1u, true);

		return resource->resourceID;
	}

	void RenderGraphBuilder::SetPassExecutionQueue(GHL::EGPUQueue queueIndex) {
		mPassNode->SetExecutionQueue(queueIndex);
	}

}