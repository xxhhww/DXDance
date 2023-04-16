#pragma once
#include "Resource.h"

#include "GHL/ResourceBarrierBatch.h"

#include <vector>
#include <unordered_map>

namespace Renderer {

	class ResourceStateTracker {
	public:
		struct SubresourceState {
		public:
			uint32_t subresourceIndex{ 0u };
			GHL::EResourceState subresourceStates{ GHL::EResourceState::Common };
		};

		using SubresourceStateList = std::vector<SubresourceState>;

	public:
		void StartTracking(Resource* resource);
		void StopTracking(Resource* resource);

		GHL::ResourceBarrierBatch TransitionImmediately(Resource* resource, GHL::EResourceState newState, bool tryImplicitly = false);
		GHL::ResourceBarrierBatch TransitionImmediately(Resource* resource, uint32_t subresourceIndex, GHL::EResourceState newState, bool tryImplicitly = false);
		GHL::ResourceBarrierBatch TransitionImmediately(Resource* resource, const SubresourceStateList& newSubresourceStates, GHL::EResourceState newState, bool tryImplicitly = false);

	private:
		SubresourceStateList& GetSubresourceStateListInternal(Resource* resource);

		bool IsNewStateRedundant(GHL::EResourceState oldState, GHL::EResourceState newState);

	private:
		std::unordered_map<Resource*, SubresourceStateList> mSubresourceStateMap;
	};

}