#include "RenderGraphItem.h"

namespace Renderer {

	void PassNode::AddReadDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceIndex, bool isBuffer) {
		readSubresources.emplace(EncodeSubresourceID(resourceID, subresourceIndex, isBuffer));
	}

	void PassNode::AddReadDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceStartIndex, uint32_t subresourceCount, bool isBuffer) {
		for (size_t index = subresourceStartIndex; index < subresourceCount; index++) {
			AddReadDependency(resourceID, index, isBuffer);
		}
	}
	
	void PassNode::AddReadDependency(const RenderGraphResourceID& resourceID, std::vector<uint32_t>&& subresourceIndexList, bool isBuffer) {
		for (const auto& index : subresourceIndexList) {
			AddReadDependency(resourceID, index, isBuffer);
		}
	}

	void PassNode::AddWriteDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceIndex, bool isBuffer) {
		writeSubresources.emplace(EncodeSubresourceID(resourceID, subresourceIndex, isBuffer));
	}
	
	void PassNode::AddWriteDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceStartIndex, uint32_t subresourceCount, bool isBuffer) {
		for (size_t index = subresourceStartIndex; index < subresourceCount; index++) {
			AddWriteDependency(resourceID, index, isBuffer);
		}
	}
	
	void PassNode::AddWriteDependency(const RenderGraphResourceID& resourceID, std::vector<uint32_t>&& subresourceIndexList, bool isBuffer) {
		for (const auto& index : subresourceIndexList) {
			AddWriteDependency(resourceID, index, isBuffer);
		}
	}

	void PassNode::SetExecutionQueue(GHL::EGPUQueue queue) {
		executionQueueIndex = std::underlying_type<GHL::EGPUQueue>::type(queue);
	}

	DependencyLevel::DependencyLevel() {
		transitionNodePerQueue.resize(std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Count));
		passNodesPerQueue.resize(std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Count));
	}

}