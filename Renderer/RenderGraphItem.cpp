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

	void PassNode::SetExecutionQueue(PassExecutionQueue queue) {
		executionQueueIndex = std::underlying_type<PassExecutionQueue>::type(queue);
	}

	DependencyLevel::DependencyLevel() {
		generalBarrierBatchPerQueue.resize(std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::Count));
		passNodesPerQueue.resize(std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::Count));
	}

}