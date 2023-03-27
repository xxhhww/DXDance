#include "RenderGraphResource.h"

namespace Renderer {

	void RenderGraphResource::ApplyInitialStates(GHL::EResourceState initialState) {
		mInitialStates |= initialState;
	}

	void RenderGraphResource::ApplyExpectedStates(GHL::EResourceState expectedState) {
		mExpectedStates |= expectedState;
	}

	void RenderGraphResource::StartTimeline(uint64_t nodeExecutionIndex) {
		mTimeline.first = mTimeline.second = nodeExecutionIndex;
	}

	void RenderGraphResource::UpdateTimeline(uint64_t nodeExecutionIndex) {
		mTimeline.second = nodeExecutionIndex;
	}

}