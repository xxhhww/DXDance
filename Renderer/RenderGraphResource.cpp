#include "RenderGraphResource.h"
#include "Tools/VisitorHelper.h"
#include "Texture.h"
#include "Buffer.h"

namespace Renderer {

	RenderGraphResource::RenderGraphResource(const GHL::Device* device, const std::string& name)
	: mDevice(device)
	, mResName(name)
	, imported(false)
	, resourceFormat(device) {}

	RenderGraphResource::RenderGraphResource(const std::string& name, Texture* resource)
	: mResName(name)
	, imported(true)
	, texture(resource) 
	, resourceFormat(texture->GetResourceFormat()) {}

	RenderGraphResource::RenderGraphResource(const std::string& name, Buffer* resource)
	: mResName(name)
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
				desc.initialState = mInitialStates;
				desc.expectedState = mExpectedStates;
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
				desc.initialState = mInitialStates;
				desc.expectedState = mExpectedStates;

				resourceFormat.SetBufferDesc(desc);
			})
			, newResourceProperties);

		resourceFormat.Build();
	}

	void RenderGraphResource::StartTimeline(uint64_t nodeGlobalExecutionIndex) {
		mTimeline.first = mTimeline.second = nodeGlobalExecutionIndex;
	}

	void RenderGraphResource::UpdateTimeline(uint64_t nodeGlobalExecutionIndex) {
		mTimeline.second = nodeGlobalExecutionIndex;
	}

	void RenderGraphResource::SetInitialStates(GHL::EResourceState states) {
		mInitialStates |= states;
	}

	void RenderGraphResource::SetExpectedStates(uint64_t nodeIndex, GHL::EResourceState states) {
		mExpectedStates |= states;
		if (mRequestedInfoPerPass.find(nodeIndex) == mRequestedInfoPerPass.end()) {
			mRequestedInfoPerPass[nodeIndex].expectedStates = states;
		}
		else {
			mRequestedInfoPerPass[nodeIndex].expectedStates |= states;
		}
	}

	void RenderGraphResource::SetAliasedForFirstPass() {
		auto& firstPassInfo = mRequestedInfoPerPass.begin()->second;
		firstPassInfo.needAliased = true;

		aliased = true;
	}

}