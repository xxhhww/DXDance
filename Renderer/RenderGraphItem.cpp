#include "RenderGraphItem.h"

namespace Renderer {

	void PassNode::AddReadDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceIndex) {
		readSubresources.emplace(EncodeSubresourceID(resourceID, subresourceIndex));
	}

	void PassNode::AddReadDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceStartIndex, uint32_t subresourceCount) {
		for (size_t index = subresourceStartIndex; index < subresourceCount; index++) {
			AddReadDependency(resourceID, index);
		}
	}
	
	void PassNode::AddReadDependency(const RenderGraphResourceID& resourceID, std::vector<uint32_t>&& subresourceIndexList) {
		for (const auto& index : subresourceIndexList) {
			AddReadDependency(resourceID, index);
		}
	}

	void PassNode::AddWriteDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceIndex) {
		writeSubresources.emplace(EncodeSubresourceID(resourceID, subresourceIndex));
	}
	
	void PassNode::AddWriteDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceStartIndex, uint32_t subresourceCount) {
		for (size_t index = subresourceStartIndex; index < subresourceCount; index++) {
			AddWriteDependency(resourceID, index);
		}
	}
	
	void PassNode::AddWriteDependency(const RenderGraphResourceID& resourceID, std::vector<uint32_t>&& subresourceIndexList) {
		for (const auto& index : subresourceIndexList) {
			AddWriteDependency(resourceID, index);
		}
	}

	void PassNode::SetExecutionQueue(GHL::EGPUQueue queue) {
		executionQueueIndex = std::underlying_type<GHL::EGPUQueue>::type(queue);
	}

	DependencyLevel::DependencyLevel() {
		passNodesPerQueue.resize(std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Count));
	}

}