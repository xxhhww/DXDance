#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResource.h"
#include "RenderGraphResourceStorage.h"
#include "RenderGraphItem.h"
#include "RenderGraphPass.h"

#include <sstream>

namespace Renderer {

	RenderGraph::RenderGraph(const GHL::Device* device, RingFrameTracker* frameTracker, PoolDescriptorAllocator* descriptorAllocator)
	: mFrameTracker(frameTracker) 
	, mResourceStorage(std::make_unique<RenderGraphResourceStorage>(device, descriptorAllocator)) {}

	void RenderGraph::Build() {
		if (mCompiled) {
			return;
		}

		// SetUp
		for (auto& passNode : mPassNodes) {
			RenderGraphBuilder builder(passNode.get(), mResourceStorage.get());
			passNode->renderPass->SetUp(builder);
		}

		BuildAdjacencyList();
		
		TopologicalSort();

		BuildDependencyLevels();

		BuildAliasingBarrier();

		BuildGeneralBarrier();

		mCompiled = true;
	}

	void RenderGraph::Execute() {

	}

	void RenderGraph::ImportResource(const std::string& name, Buffer* importedBuffer) {
		mResourceStorage->ImportResource(name, importedBuffer);
	}

	void RenderGraph::ImportResource(const std::string& name, Texture* importedTexture) {
		mResourceStorage->ImportResource(name, importedTexture);
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
				for (const auto& readSubresourceID : otherPassNode->readSubresources) {

					// 发现资源依赖
					if (currPassNode->writeSubresources.find(readSubresourceID) != currPassNode->writeSubresources.end()) {
						// 存储邻接表
						mAdjacencyLists.at(currIndex).push_back(otherIndex);
						
						// 存储GraphEdge
						// mGraphEdges.emplace_back(currIndex, otherIndex, currPassNode->executionQueueIndex != otherPassNode->executionQueueIndex);
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
				DepthFirstSearch(i, visited, stack);
			}
		}

		while (!stack.empty()) {
			uint64_t index = stack.top();
			mSortedPassNodes.push_back(index);
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
		std::vector<uint64_t> longestDistances(mSortedPassNodes.size(), 0u);

		uint64_t dependencyLevelCount = 1u;

		for (size_t i = 0u; i < mSortedPassNodes.size(); i++) {

			uint64_t nodeIndex = mSortedPassNodes.at(i);

			auto& adjacencyList = mAdjacencyLists.at(nodeIndex);

			for (const auto& adjIndex : adjacencyList) {
				if (longestDistances.at(adjIndex) < longestDistances.at(nodeIndex) + 1u) {
					longestDistances.at(adjIndex) = longestDistances.at(nodeIndex) + 1u;
					
					dependencyLevelCount = std::max(dependencyLevelCount, longestDistances.at(adjIndex) + 1u);

				}
			}
		}

		for (size_t i = 0; i < dependencyLevelCount; i++) {
			mDependencyLevelList.emplace_back(std::make_unique<DependencyLevel>());
			mDependencyLevelList.back()->levelIndex = i;
		}

		for (size_t i = 0u; i < mSortedPassNodes.size(); i++) {

			uint64_t passNodeIndex = mSortedPassNodes.at(i);

			auto& passNode = mPassNodes.at(passNodeIndex);
			auto& passDependencyLevel = longestDistances.at(passNodeIndex);

			auto& dependencyLevel = mDependencyLevelList.at(passDependencyLevel);
			dependencyLevel->passNodesPerQueue.at(passNode->executionQueueIndex).push_back(passNode.get());

			passNode->dependencyLevelIndex = passDependencyLevel;
			passNode->localToQueueExecutionIndexWithoutBarrier = mPassNodesPerQueue.at(passNode->executionQueueIndex).size();

			/*
			// 向GraphEdge中添加由在Queue上的执行顺序所形成的依赖
			if (passNode->localToQueueExecutionIndexWithoutBarrier > 0u) {
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
			*/

			mPassNodesPerQueue.at(passNode->executionQueueIndex).push_back(passNodeIndex);

			/*
			// 更新资源的生命周期
			for (const auto& resName : passNode->writeDependency) {
				auto* resource = mResourceStorage->GetResource(resName);
				resource->StartTimeline(passNode->globalExecutionIndex);
			}

			for (const auto& resName : passNode->readDependency) {
				auto* resource = mResourceStorage->GetResource(resName);
				resource->UpdateTimeline(passNode->globalExecutionIndex);
			}
			*/
		}
	}

	void RenderGraph::CullRedundantDependencies() {
		/*
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
		*/
	}

	void RenderGraph::BuildGeneralBarrier() {

	}

	void RenderGraph::BuildAliasingBarrier() {

	}

}