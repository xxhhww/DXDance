#pragma once
#include "RenderGraphResource.h"

#include "GHL/pbh.h"
#include "GHL/ResourceBarrierBatch.h"

#include <vector>
#include <unordered_map>
#include <optional>

namespace Renderer {

	class RenderGraphResourceStateTracker {
	public:
		struct SubresourceState {
		public:
			uint32_t subresourceIndex{ 0u };
			GHL::EResourceState subresourceStates{ GHL::EResourceState::Common };
		};

		using SubresourceStateList = std::vector<SubresourceState>;

	public:
		void StartTracking(RenderGraphResource* resource);
		void StopTracking(RenderGraphResource* resource);

		void ResetInitialStates();

		std::optional<GHL::ResourceBarrier> TransitionImmediately(RenderGraphResource* resource, GHL::EResourceState newState, bool tryImplicitly = false);
		std::optional<GHL::ResourceBarrier> TransitionImmediately(RenderGraphResource* resource, uint32_t subresourceIndex, GHL::EResourceState newState, bool tryImplicitly = false);

	private:
		std::unordered_map<RenderGraphResource*, SubresourceStateList> mSubresourceStateMap;
	};

}