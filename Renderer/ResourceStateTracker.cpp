#include "ResourceStateTracker.h"
#include "RenderGraphResource.h"

#include "Tools/Assert.h"

#include "Texture.h"

namespace Renderer {

	void ResourceStateTracker::StartTracking(Resource* resource) {
		if (mSubresourceStateMap.find(resource) != mSubresourceStateMap.end()) {
			ASSERT_FORMAT(false, "Resource Already Exists");
		}

		mSubresourceStateMap[resource] = SubresourceStateList{};
		mSubresourceStateMap[resource].resize(resource->GetResourceFormat().SubresourceCount());

		for (uint32_t i = 0; i < resource->GetResourceFormat().SubresourceCount(); i++) {
			mSubresourceStateMap.at(resource).at(i).subresourceIndex = i;
			mSubresourceStateMap.at(resource).at(i).subresourceStates = resource->GetResourceFormat().GetInitialState();
		}
	}

	void ResourceStateTracker::StopTracking(Resource* resource) {
		auto it = mSubresourceStateMap.find(resource);
		if (it != mSubresourceStateMap.end()) {
			mSubresourceStateMap.erase(it);
		}
	}

	GHL::ResourceBarrierBatch ResourceStateTracker::TransitionImmediately(Resource* resource, GHL::EResourceState newState, bool tryImplicitly) {
		SubresourceStateList& currentSubresourceStates = GetSubresourceStateListInternal(resource);
		
		GHL::ResourceBarrierBatch barrierBatch{};
		for (uint32_t subresourceIndex = 0; subresourceIndex < currentSubresourceStates.size(); subresourceIndex++) {
			GHL::EResourceState oldState = currentSubresourceStates[subresourceIndex].subresourceStates;
			currentSubresourceStates[subresourceIndex].subresourceStates = newState;

			if (IsNewStateRedundant(oldState, newState)) {
				continue;
			}

			barrierBatch.AddBarrier(GHL::TransitionBarrier{ static_cast<GHL::Resource*>(resource), oldState, newState, subresourceIndex });
		}

		return barrierBatch;
	}

	GHL::ResourceBarrierBatch ResourceStateTracker::TransitionImmediately(Resource* resource, uint32_t subresourceIndex, GHL::EResourceState newState, bool tryImplicitly) {
		SubresourceStateList& currentSubresourceStates = GetSubresourceStateListInternal(resource);
		ASSERT_FORMAT(subresourceIndex < currentSubresourceStates.size(), "Requested a state change for subresource that doesn't exist");
		GHL::EResourceState oldState = currentSubresourceStates[subresourceIndex].subresourceStates;
		currentSubresourceStates[subresourceIndex].subresourceStates = newState;

		if (IsNewStateRedundant(oldState, newState)) {
			return GHL::ResourceBarrierBatch{};
		}
		/*
		if (CanTransitionToStateImplicitly(resource, oldState, newState, tryApplyImplicitly)) {
			return GHL::ResourceBarrierBatch{};
		}
		*/
		return GHL::TransitionBarrier { static_cast<GHL::Resource*>(resource), oldState, newState, subresourceIndex };
	}

	GHL::ResourceBarrierBatch ResourceStateTracker::TransitionImmediately(Resource* resource, const SubresourceStateList& newSubresourceStates, GHL::EResourceState newState, bool tryImplicitly) {
		return GHL::ResourceBarrierBatch{};
	}

	ResourceStateTracker::SubresourceStateList& ResourceStateTracker::GetSubresourceStateListInternal(Resource* resource) {
		auto it = mSubresourceStateMap.find(resource);
		ASSERT_FORMAT(it != mSubresourceStateMap.end(), "Resource is not registered / not being tracked");
		return it->second;
	}

	bool ResourceStateTracker::IsNewStateRedundant(GHL::EResourceState oldState, GHL::EResourceState newState) {
		// return (oldState == newState) || (GHL::IsRWResourceState(oldState) && GHL::IsWriteResourceState(newState));
		return (oldState == newState);
	}

}