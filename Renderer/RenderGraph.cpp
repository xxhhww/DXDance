#include "RenderGraph.h"

namespace Renderer {

	RenderGraph::RenderGraph(RingFrameTracker* frameTracker)
	:mFrameTracker(frameTracker) {}

	void RenderGraph::PassNode::SetExpectedState(const std::string& name, GHL::EResourceState expectedState) {

		if (expectedStateMap.find(name) == expectedStateMap.end()) {
			expectedStateMap[name] = expectedState;
		}
		expectedStateMap[name] |= expectedState;
	}

	void RenderGraph::PassNode::SetExecutionQueue(PassExecutionQueue queueIndex) {
		executionQueueIndex = queueIndex;
	}

	void RenderGraph::Build() {

	}

	void RenderGraph::Execute() {

	}

}