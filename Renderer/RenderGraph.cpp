#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResource.h"
#include "RenderGraphResourceStorage.h"
#include "RenderGraphItem.h"
#include "RenderGraphPass.h"
#include "ResourceStateTracker.h"

#include "Tools/Assert.h"

namespace Renderer {

	RenderGraph::RenderGraph(const GHL::Device* device, RingFrameTracker* frameTracker, PoolDescriptorAllocator* descriptorAllocator)
	: mFrameTracker(frameTracker) 
	, mResourceStorage(std::make_unique<RenderGraphResourceStorage>(device, descriptorAllocator)) 
	, mResourceStateTracker(std::make_unique<RenderGraphResourceStateTracker>()) {}

	void RenderGraph::Build() {
		if (mCompiled) {
			return;
		}

		SetupInternalResource();

		BuildAdjacencyList();
		
		TopologicalSort();

		BuildDependencyLevels();

		BuildTransitionBarrier();

		BuildAliasingBarrier();

		mCompiled = true;
	}

	void RenderGraph::Execute() {

	}

	void RenderGraph::ImportResource(const std::string& name, Buffer* importedBuffer) {
		auto* resource = mResourceStorage->ImportResource(name, importedBuffer);
		mResourceStateTracker->StartTracking(resource);
	}

	void RenderGraph::ImportResource(const std::string& name, Texture* importedTexture) {
		auto* resource = mResourceStorage->ImportResource(name, importedTexture);
		mResourceStateTracker->StartTracking(resource);
	}

	void RenderGraph::SetupInternalResource() {
		// Set up Internal Resource
		for (auto& passNode : mPassNodes) {
			RenderGraphBuilder builder(passNode.get(), mResourceStorage.get());
			passNode->renderPass->SetUp(builder);
		}
		// Start Tracking Internal Resource
		for (auto& pair : mResourceStorage->GetResources()) {
			if (!pair.second->imported) {
				mResourceStateTracker->StartTracking(pair.second.get());
			}
		}
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
			dependencyLevel->passNodes.push_back(passNode.get());

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

	void RenderGraph::BuildTransitionBarrier() {
		
		// 转换屏障重布线的几种情况:
		// 1. 目标资源在当前DL中存在跨队列读取
		// 2. 该资源转换屏障的前后状态不被目标队列所支持(例如: 计算队列不支持PixelShaderAccess)
		for (size_t i = 0; i < mDependencyLevelList.size(); i++) {
			auto* currDL = mDependencyLevelList.at(i).get();
			auto* lastDL = (i == 0) ? nullptr : mDependencyLevelList.at(i - 1u).get();

			std::unordered_set<SubresourceID> visitedSubresourceIDs;

			for (auto* currPassNode : currDL->passNodes) {

				// process read subresource
				for (auto& readSubresourceID : currPassNode->readSubresources) {
					if (visitedSubresourceIDs.find(readSubresourceID) != visitedSubresourceIDs.end()) {
						// 当前DL内，已经有其他节点访问过该子资源了
						visitedSubresourceIDs.insert(readSubresourceID);
						continue;
					}

					// 解算SubresourceID并获得对应的RenderGraphResource
					auto [resourceID, subresourceIndex, isBuffer] = DecodeSubresourceID(readSubresourceID);
					auto* resource = mResourceStorage->GetResourceByID(resourceID);

					// 该Resource需求的队列
					std::unordered_set<uint8_t> queuesRequiredForResource;
					queuesRequiredForResource.insert(currPassNode->executionQueueIndex);

					GHL::EResourceState expectedStatesInCurrDL{ GHL::EResourceState::Common };
					bool readByMutipleQueues{ false };
					bool needReroutingOnAllQueues{ false };

					expectedStatesInCurrDL = resource->GetSubresourceRequestedInfo(currPassNode->passNodeIndex, subresourceIndex);

					// 收集CurrDL中其他所有PassNode对该资源的请求
					for (auto* otherPassNode : currDL->passNodes) {
						if (currPassNode == otherPassNode) {
							continue;
						}

						// 该DL中的其他PassNode与当前的PassNode发生了读写冲突
						ASSERT_FORMAT(otherPassNode->writeSubresources.find(readSubresourceID) == otherPassNode->writeSubresources.end(),
							"A Read Write Conflict Has Occurred");

						if (otherPassNode->readSubresources.find(readSubresourceID) == otherPassNode->readSubresources.end()) {
							continue;
						}

						// otherPassNode也读取该Resource

						expectedStatesInCurrDL |= resource->GetSubresourceRequestedInfo(otherPassNode->passNodeIndex, subresourceIndex);
						
						if (queuesRequiredForResource.find(otherPassNode->executionQueueIndex) != queuesRequiredForResource.end()) {
							continue;
						}

						queuesRequiredForResource.insert(otherPassNode->executionQueueIndex);
						readByMutipleQueues = true;
					}
 
					std::optional<GHL::ResourceBarrier> barrier = mResourceStateTracker->TransitionImmediately(resource, expectedStatesInCurrDL, true);
					if (!barrier) {
						// 该资源在CurrDL的状态与LastDL不变
						continue;
					}

					if (readByMutipleQueues) {
						// 资源被多队列读取，检测一个是否存在一个队列支持资源的前后状态
						std::optional<uint8_t> availableQueueIndex{ std::nullopt };
						availableQueueIndex = FindClosestQueue();


						if (!availableQueueIndex) {
							// 当这些队列都不可用时，需要进行更大范围的重布线
							needReroutingOnAllQueues = true;
						}
						else {
							// 仅在这些队列内部进行重布线
						}
					}
					else {
						// 资源仅被一个队列读取

						if (!IsStatesSupportedOnQueue()) {
							needReroutingOnAllQueues = true;
						}
						else {

						}
						
					}



					visitedSubresourceIDs.insert(readSubresourceID);
				}

				// process write subresource
				for (auto& writeSubresourceID : currPassNode->writeSubresources) {


					visitedSubresourceIDs.insert(writeSubresourceID);
				}

			}

		}

	}

	void RenderGraph::BuildAliasingBarrier() {

	}

}