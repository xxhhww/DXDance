#include "RenderGraphResource.h"

namespace Renderer {

	RenderGraphResource::RenderGraphResource(const GHL::Device* device, const std::string& name)
	: mDevice(device)
	, mResName(name)
	, mImported(false) {}

	RenderGraphResource::RenderGraphResource(const std::string& name, Texture* resource)
	: mResName(name)
	, mImported(true)
	, mTexture(resource) {}

	RenderGraphResource::RenderGraphResource(const std::string& name, Buffer* resource)
	: mResName(name)
	, mImported(true)
	, mBuffer(resource) {}

	void RenderGraphResource::StartTimeline(uint64_t nodeGlobalExecutionIndex) {
		mTimeline.first = mTimeline.second = nodeGlobalExecutionIndex;
	}

	void RenderGraphResource::UpdateTimeline(uint64_t nodeGlobalExecutionIndex) {
		mTimeline.second = nodeGlobalExecutionIndex;
	}

	void RenderGraphResource::SetExpectedStates(uint64_t nodeIndex, GHL::EResourceState states) {
		if (mExpectedStatesPerPass.find(nodeIndex) == mExpectedStatesPerPass.end()) {
			mExpectedStatesPerPass[nodeIndex] = states;
		}
		else {
			mExpectedStatesPerPass[nodeIndex] |= states;
		}
	}

}