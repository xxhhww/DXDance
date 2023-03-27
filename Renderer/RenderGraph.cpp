#include "RenderGraph.h"

namespace Renderer {

	RenderGraph::RenderGraph(RingFrameTracker* frameTracker)
	:mFrameTracker(frameTracker) {
		mPassCountPerQueue.resize(std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::Count), 0u);
	}

	void RenderGraph::PassNode::SetExpectedState(const std::string& name, GHL::EResourceState expectedState) {

		if (expectedStateMap.find(name) == expectedStateMap.end()) {
			expectedStateMap[name] = expectedState;
		}
		expectedStateMap[name] |= expectedState;
	}

	void RenderGraph::PassNode::AddReadDependency(const std::string& name) {
		if (readDependency.find(name) != readDependency.end()) {
			return;
		}
		readDependency.insert(name);
	}

	void RenderGraph::PassNode::AddWriteDependency(const std::string& name) {
		if (writeDependency.find(name) != writeDependency.end()) {
			return;
		}
		writeDependency.insert(name);
	}

	void RenderGraph::PassNode::SetExecutionQueue(PassExecutionQueue queueIndex) {
		executionQueueIndex = std::underlying_type<PassExecutionQueue>::type(queueIndex);
	}

	void RenderGraph::DependencyLevel::AddNode(PassNode* passNode) {
		mPassNodes.push_back(passNode);

		mPassNodesPerQueue.at(passNode->executionQueueIndex).push_back(passNode);
	}

	RenderGraph::DependencyLevel::DependencyLevel() {
		mPassNodesPerQueue.resize(std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::Count));
	}

	void RenderGraph::DependencyLevel::SetLevel(uint64_t level) {
		mLevelIndex = level;
	}

	void RenderGraph::Build() {

	}

	void RenderGraph::Execute() {

	}

	void RenderGraph::ImportResource(Buffer* importedBuffer) {

	}

	void RenderGraph::ImportResource(Texture* importedTexture) {

	}

	void RenderGraph::BuildAdjacencyList() {
		mAdjacencyLists.resize(mPassNodes.size());

		for (size_t currIndex = 0; currIndex < mPassNodes.size(); currIndex++) {

			auto& currPassNode = mPassNodes.at(currIndex);

			// 遍历其他所有的PassNode
			for (size_t otherIndex = 0; otherIndex < mPassNodes.size(); otherIndex++) {
				if (currIndex == otherIndex) continue;

				auto& otherPassNode = mPassNodes.at(otherIndex);

				// 遍历otherPassNode的所有读资源，判断它是否依赖于currPassNode的写资源
				for (const auto& readResourceName : otherPassNode->readDependency) {

					// 发现资源依赖
					if (currPassNode->writeDependency.find(readResourceName) != currPassNode->writeDependency.end()) {
						mAdjacencyLists.at(currIndex).push_back(otherIndex);

						// 存在资源的跨队列依赖
						if (currPassNode->executionQueueIndex != otherPassNode->executionQueueIndex) {
							currPassNode->syncSignalRequired = true;
							otherPassNode->nodesToSyncWith.push_back(currPassNode.get());
						}

					}
				}
			}
		}
	}

	void RenderGraph::TopologicalSort() {
		std::stack<uint64_t> stack;
		std::vector<bool> visited(mPassNodes.size());

		for (size_t i = 0; i < mPassNodes.size(); i++) {
			if (!visited.at(i)) {

			}
		}

		while (!stack.empty()) {
			uint64_t index = stack.top();
			mSortedPassNodes.push_back(mPassNodes.at(index).get());
			stack.pop();
		}
	}

	void RenderGraph::DepthFirstSearch(uint64_t nodeIndex, std::vector<bool>& visited, std::stack<uint64_t>& stack) {
		visited.at(nodeIndex) = true;

		auto& adjacencyList = mAdjacencyLists.at(nodeIndex);

		for (const auto& index : adjacencyList) {
			if (!visited.at(index)) {
				DepthFirstSearch(index, visited, stack);
			}
		}

		stack.push(nodeIndex);
	}

	void RenderGraph::BuildDependencyLevels() {
		std::vector<uint64_t> longestDistances(mPassNodes.size(), 0u);

		uint64_t dependencyLevelCount = 1u;

		for (size_t nodeIndex = 0u; nodeIndex < mSortedPassNodes.size(); nodeIndex++) {

			auto& adjacencyList = mAdjacencyLists.at(nodeIndex);

			for (const auto& adjIndex : adjacencyList) {
				if (longestDistances.at(adjIndex) < longestDistances.at(nodeIndex) + 1u) {
					longestDistances.at(adjIndex) = longestDistances.at(nodeIndex) + 1u;
					
					dependencyLevelCount = std::max(dependencyLevelCount, longestDistances.at(adjIndex));

				}
			}
		}

		mDependencyList.resize(dependencyLevelCount);

		for (size_t nodeIndex = 0u; nodeIndex < mSortedPassNodes.size(); nodeIndex++) {

			auto* passNode = mSortedPassNodes.at(nodeIndex);
			auto& passDepthLevel = longestDistances.at(nodeIndex);

			auto& dependencyLevel = mDependencyList.at(passDepthLevel);
			dependencyLevel.SetLevel(passDepthLevel);
			dependencyLevel.AddNode(passNode);

			passNode->dependencyLevelIndex = passDepthLevel;
			passNode->globalExecutionIndex = nodeIndex;
			passNode->localToDependencyLevelExecutionIndex = dependencyLevel.GetNodeSize() - 1u;
			passNode->localToQueueExecutionIndex = mPassCountPerQueue.at(passNode->executionQueueIndex);

			mPassCountPerQueue.at(passNode->executionQueueIndex)++;

		}
	}

	void RenderGraph::TraverseDependencyLevels() {

		for (auto& dependencyLevel : mDependencyList) {

			auto& passNodes = dependencyLevel.GetPassNodes();

			for (auto* passNode : passNodes) {

			}

		}

	}

}