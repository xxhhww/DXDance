#include "RenderGraphResource.h"


namespace Renderer {

	RenderGraphResource::RenderGraphResource(const GHL::Device* device, const std::string& name)
	: mDevice(device)
	, mResName(name)
	, mImported(false) 
	, mResourceFormat(device) {}

	RenderGraphResource::RenderGraphResource(const std::string& name, Texture* resource)
	: mResName(name)
	, mImported(true)
	, mTexture(resource) {}

	RenderGraphResource::RenderGraphResource(const std::string& name, Buffer* resource)
	: mResName(name)
	, mImported(true)
	, mBuffer(resource) {}

	void RenderGraphResource::BuildResourceFormat() {

	}

	void RenderGraphResource::StartTimeline(uint64_t nodeGlobalExecutionIndex) {
		mTimeline.first = mTimeline.second = nodeGlobalExecutionIndex;
	}

	void RenderGraphResource::UpdateTimeline(uint64_t nodeGlobalExecutionIndex) {
		mTimeline.second = nodeGlobalExecutionIndex;
	}

	void RenderGraphResource::SetNewTextureProperties(const NewTextureProperties& properties) {
		mNewResourceProperties = properties;
	}

	void RenderGraphResource::SetNewBufferProperties(const NewBufferProperties& properties) {
		mNewResourceProperties = properties;
	}

	void RenderGraphResource::SetInitialStates(GHL::EResourceState states) {
		mInitialStates |= states;
	}

	void RenderGraphResource::SetExpectedStates(uint64_t nodeIndex, GHL::EResourceState states) {
		mExpectedStates |= states;
		if (mExpectedStatesPerPass.find(nodeIndex) == mExpectedStatesPerPass.end()) {
			mExpectedStatesPerPass[nodeIndex] = states;
		}
		else {
			mExpectedStatesPerPass[nodeIndex] |= states;
		}
	}

}