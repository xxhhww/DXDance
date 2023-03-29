#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include <sstream>

namespace Renderer {

	RenderGraph::RenderGraph(RingFrameTracker* frameTracker)
	:mFrameTracker(frameTracker) {
		mGraphNodesPerQueue.resize(std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::Count));
	}

	void RenderGraph::GraphNode::AddReadDependency(const std::string& name) {
		if (readDependency.find(name) != readDependency.end()) {
			return;
		}
		readDependency.insert(name);
	}

	void RenderGraph::GraphNode::AddWriteDependency(const std::string& name) {
		if (writeDependency.find(name) != writeDependency.end()) {
			return;
		}
		writeDependency.insert(name);
	}

	void RenderGraph::GraphNode::SetExpectedStates(const std::string& name, GHL::EResourceState states) {
		if (expectedStatesMap.find(name) == expectedStatesMap.end()) {
			expectedStatesMap[name] = states;
		}
		else {
			expectedStatesMap[name] |= states;
		}
	}

	void RenderGraph::GraphNode::SetExecutionQueue(PassExecutionQueue queueIndex) {
		executionQueueIndex = std::underlying_type<PassExecutionQueue>::type(queueIndex);
	}

	RenderGraph::GraphEdge::GraphEdge(uint64_t producerNodeIndex, uint64_t consumerNodeIndex, bool crossQueue)
	: producerNodeIndex(producerNodeIndex)
	, consumerNodeIndex(consumerNodeIndex)
	, crossQueue(crossQueue) {}

	void RenderGraph::DependencyLevel::AddNode(GraphNode* passNode) {
		mGraphNodes.push_back(passNode);

		mGraphNodesPerQueue.at(passNode->executionQueueIndex).push_back(passNode);
	}

	RenderGraph::DependencyLevel::DependencyLevel() {
		mGraphNodesPerQueue.resize(std::underlying_type<PassExecutionQueue>::type(PassExecutionQueue::Count));
	}

	void RenderGraph::DependencyLevel::SetLevel(uint64_t level) {
		mLevelIndex = level;
	}

	void RenderGraph::Build() {
		if (mCompiled) {
			return;
		}

		// SetUp
		for (auto& graphNode : mGraphNodes) {
			RenderGraphBuilder builder{ graphNode.get(), this };
			graphNode->pass->SetUp(builder);
		}

		BuildAdjacencyList();
		TopologicalSort();
		BuildDependencyLevels();
		CullRedundantDependencies();


		mCompiled = true;
	}

	void RenderGraph::Execute() {

	}

	void RenderGraph::ImportResource(Buffer* importedBuffer) {

	}

	void RenderGraph::ImportResource(Texture* importedTexture) {

	}

	void RenderGraph::BuildAdjacencyList() {
		mAdjacencyLists.resize(mGraphNodes.size());

		for (size_t currIndex = 0; currIndex < mGraphNodes.size(); currIndex++) {

			auto& currPassNode = mGraphNodes.at(currIndex);

			// 遍历其他所有的PassNode
			for (size_t otherIndex = 0; otherIndex < mGraphNodes.size(); otherIndex++) {
				if (currIndex == otherIndex) continue;

				auto& otherPassNode = mGraphNodes.at(otherIndex);

				// 遍历otherPassNode的所有读资源，判断它是否依赖于currPassNode的写资源
				for (const auto& readResourceName : otherPassNode->readDependency) {

					// 发现资源依赖
					if (currPassNode->writeDependency.find(readResourceName) != currPassNode->writeDependency.end()) {
						// 存储邻接表
						mAdjacencyLists.at(currIndex).push_back(otherIndex);
						// 存储GraphEdge
						mGraphEdges.emplace_back(currIndex, otherIndex, currPassNode->executionQueueIndex != otherPassNode->executionQueueIndex);
					}
				}
			}
		}
	}

	void RenderGraph::TopologicalSort() {
		std::stack<uint64_t> stack;
		std::vector<bool> visited(mGraphNodes.size());

		for (size_t i = 0; i < mGraphNodes.size(); i++) {
			if (!visited.at(i)) {
				DepthFirstSearch(i, visited, stack);
			}
		}

		while (!stack.empty()) {
			uint64_t index = stack.top();
			mSortedGraphNodes.push_back(index);
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
		std::vector<uint64_t> longestDistances(mSortedGraphNodes.size(), 0u);

		uint64_t dependencyLevelCount = 1u;

		for (size_t i = 0u; i < mSortedGraphNodes.size(); i++) {

			uint64_t nodeIndex = mSortedGraphNodes.at(i);

			auto& adjacencyList = mAdjacencyLists.at(nodeIndex);

			for (const auto& adjIndex : adjacencyList) {
				if (longestDistances.at(adjIndex) < longestDistances.at(nodeIndex) + 1u) {
					longestDistances.at(adjIndex) = longestDistances.at(nodeIndex) + 1u;
					
					dependencyLevelCount = std::max(dependencyLevelCount, longestDistances.at(adjIndex) + 1u);

				}
			}
		}

		mDependencyList.resize(dependencyLevelCount);

		for (size_t i = 0u; i < mSortedGraphNodes.size(); i++) {

			uint64_t nodeIndex = mSortedGraphNodes.at(i);

			auto& passNode = mGraphNodes.at(nodeIndex);
			auto& passDependencyLevel = longestDistances.at(nodeIndex);

			auto& dependencyLevel = mDependencyList.at(passDependencyLevel);
			dependencyLevel.SetLevel(passDependencyLevel);
			dependencyLevel.AddNode(passNode.get());

			passNode->dependencyLevelIndex = passDependencyLevel;
			passNode->globalExecutionIndex = i;
			passNode->localToQueueExecutionIndex = mGraphNodesPerQueue.at(passNode->executionQueueIndex).size();

			// 向GraphEdge中添加由在Queue上的执行顺序所形成的依赖
			if (passNode->localToQueueExecutionIndex > 0u) {
				uint64_t prevNodeIndexOnQueue = mGraphNodesPerQueue.at(passNode->executionQueueIndex).back();

				auto it = std::find_if(mGraphEdges.begin(), mGraphEdges.end(),
					[&](const GraphEdge& edge) {
						if (edge.producerNodeIndex == prevNodeIndexOnQueue && edge.consumerNodeIndex == nodeIndex) {
							return true;
						}
						return false;
					});

				if (it == mGraphEdges.end()) {
					mGraphEdges.emplace_back(prevNodeIndexOnQueue, nodeIndex, false);
				}
			}

			mGraphNodesPerQueue.at(passNode->executionQueueIndex).push_back(nodeIndex);
		}
	}

	void RenderGraph::CullRedundantDependencies() {
		// 使用节点与依赖边与弗洛伊德算法做冗余剔除
		uint64_t nodeSize = mGraphNodes.size();

		std::vector<std::vector<int64_t>> dist(nodeSize, std::vector<int64_t>(nodeSize, INFINITE));

		for (uint64_t i = 0u; i < nodeSize; i++) {
			dist[i][i] = 0;
		}

		for (const auto& edge : mGraphEdges) {
			dist[edge.producerNodeIndex][edge.consumerNodeIndex] = -1;
		}

		for (uint64_t k = 0u; k < nodeSize; k++) {
			for (uint64_t i = 0u; i < nodeSize; i++) {
				for (uint64_t j = 0u; j < nodeSize; j++) {
					if (dist[i][j] > dist[i][k] + dist[k][j]) {
						dist[i][j] = dist[i][k] + dist[k][j];
					}
				}
			}
		}

		// 删除冗余的边
		mGraphEdges.erase(
			std::remove_if(mGraphEdges.begin(), mGraphEdges.end(), 
			[&](const GraphEdge& edge) {
				if (dist[edge.producerNodeIndex][edge.consumerNodeIndex] != -1) {
					std::wstringstream wss;
					wss << edge.producerNodeIndex << " : " << edge.consumerNodeIndex << "\n";
					OutputDebugStringW(wss.str().c_str());
					return true;
				}
				return false;
			}),
			mGraphEdges.end()
		);

		// 跨队列的边需要额外记录
		for (const auto& edge : mGraphEdges) {
			if (!edge.crossQueue) {
				continue;
			}

			// 边没有被剔除，并且该边跨队列
			auto& producerGraphNode = mGraphNodes.at(edge.producerNodeIndex);
			auto& consumerGraphNode = mGraphNodes.at(edge.consumerNodeIndex);

			producerGraphNode->requireSyncSignal = true;
			consumerGraphNode->nodesToSyncWait.push_back(producerGraphNode.get());
		}
	}

}