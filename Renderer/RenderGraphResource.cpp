#include "RenderGraphResource.h"
#include "Tools/VisitorHelper.h"
#include "Texture.h"
#include "Buffer.h"

namespace Renderer {

	RenderGraphResource::RenderGraphResource(const GHL::Device* device, const std::string& name)
	: resourceID(RenderGraphResourceID::FindOrCreateResourceID(name))
	, imported(false)
	, resourceFormat(device) {}

	RenderGraphResource::RenderGraphResource(const std::string& name, Texture* resource)
	: resourceID(RenderGraphResourceID::FindOrCreateResourceID(name))
	, imported(true)
	, texture(resource) 
	, resourceFormat(texture->GetResourceFormat()) {}

	RenderGraphResource::RenderGraphResource(const std::string& name, Buffer* resource)
	: resourceID(RenderGraphResourceID::FindOrCreateResourceID(name))
	, imported(true)
	, buffer(resource) 
	, resourceFormat(buffer->GetResourceFormat()) {}

	void RenderGraphResource::BuildResourceFormat() {
		if (imported) {
			return;
		}

		std::visit(MakeVisitor(
			[this](const NewTextureProperties& properties) {
				TextureDesc desc{};
				desc.dimension = properties.dimension;
				desc.width = properties.width;
				desc.height = properties.height;
				desc.depth = properties.depth;
				desc.arraySize = properties.arraySize;
				desc.mipLevals = properties.mipLevals;
				desc.sampleCount = properties.sampleCount;
				desc.format = properties.format;
				desc.usage = properties.usage;
				desc.miscFlag = properties.miscFlag;
				desc.clearVaule = properties.clearValue;
				desc.initialState = initialStates;
				desc.expectedState = expectedStates;
				desc.supportStream = false;

				resourceFormat.SetTextureDesc(desc);
			},
			[this](const NewBufferProperties& properties) {
				BufferDesc desc{};
				desc.stride = properties.stride;
				desc.size = properties.size;
				desc.format = properties.format;
				desc.usage = properties.usage;
				desc.miscFlag = properties.miscFlag;
				desc.initialState = initialStates;
				desc.expectedState = expectedStates;

				resourceFormat.SetBufferDesc(desc);
			})
			, newResourceProperties);

		resourceFormat.Build();
	}

	void RenderGraphResource::SetSubresourceRequestedInfo(uint64_t passNodeIndex, uint32_t subresourceIndex, GHL::EResourceState subresourceExpectedStates) {
		if (requestedInfoPerPass.find(passNodeIndex) == requestedInfoPerPass.end()) {
			requestedInfoPerPass[passNodeIndex] = PassRequestedInfo{};
		}

		auto& passRequestedInfo = requestedInfoPerPass.at(passNodeIndex);
		auto& subresourceRequestedInfos = passRequestedInfo.subresourceRequestedInfos;

		if (subresourceRequestedInfos.empty()) {
			subresourceRequestedInfos.resize(resourceFormat.SubresourceCount());
		}

		subresourceRequestedInfos.at(subresourceIndex).expectedStates |= subresourceExpectedStates;

		expectedStates |= subresourceExpectedStates;
	}

}