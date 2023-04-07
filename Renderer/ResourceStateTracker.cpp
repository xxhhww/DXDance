#include "ResourceStateTracker.h"
#include "RenderGraphResource.h"
#include "Tools/Assert.h"

namespace Renderer {

	void RenderGraphResourceStateTracker::StartTracking(RenderGraphResource* resource) {
		
		if (mSubresourceStateMap.find(resource) != mSubresourceStateMap.end()) {
			ASSERT_FORMAT(false, "Resource Already Exists");
		}

		mSubresourceStateMap[resource] = SubresourceStateList{};
		mSubresourceStateMap[resource].resize(resource->resourceFormat.SubresourceCount());

		for (uint32_t i = 0; i < resource->resourceFormat.SubresourceCount(); i++) {
			mSubresourceStateMap.at(resource).at(i).subresourceIndex = i;
			mSubresourceStateMap.at(resource).at(i).subresourceStates = resource->initialStates;
		}

	}

	void RenderGraphResourceStateTracker::StopTracking(RenderGraphResource* resource) {
		
		auto it = mSubresourceStateMap.find(resource);
		if (it != mSubresourceStateMap.end()) {
			mSubresourceStateMap.erase(it);
		}

	}

	std::optional<GHL::ResourceBarrier> RenderGraphResourceStateTracker::TransitionImmediately(RenderGraphResource* resource, GHL::EResourceState newState, bool tryImplicitly) {

	}

	std::optional<GHL::ResourceBarrier> RenderGraphResourceStateTracker::TransitionImmediately(RenderGraphResource* resource, GHL::EResourceState newState, uint32_t subresourceIndex, bool tryImplicitly) {

	}

}