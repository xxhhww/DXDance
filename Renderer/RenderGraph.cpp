#include "RenderGraph.h"
#include "RenderGraphBuilder.h"
#include "RenderGraphResource.h"
#include "RenderGraphResourceStorage.h"
#include "RenderGraphItem.h"
#include "RenderGraphPass.h"
#include "ResourceStateTracker.h"

#include "PoolCommandListAllocator.h"
#include "RingFrameTracker.h"

#include "GHL/CommandQueue.h"

#include "Tools/Assert.h"
#include "Tools/TaskProxy.h"

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

		BuildRenderGraphEdge();

		BuildAliasingBarrier();

		mCompiled = true;
	}

	void RenderGraph::Execute() {

		for (size_t i = 0; i < mDependencyLevelList.size(); i++) {

			auto* currDL = mDependencyLevelList.at(i).get();

			for (uint8_t queueIndex = 0u; queueIndex < std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Count); queueIndex++) {
				{
					// 做资源转换操作
					GHL::ResourceBarrierBatch resourceBarrierBatch;
					auto commandList = mCommandListAllocator->AllocateCommandList((GHL::EGPUQueue)queueIndex);
					auto* transitionNode = currDL->transitionNodePerQueue.at(queueIndex);
					for (const auto& [subresourceID, expectedStates] : transitionNode->expectedSubresourceStatesMap) {
						auto [resourceID, subresourceIndex, isBuffer] = DecodeSubresourceID(subresourceID);
						auto* resource = mResourceStorage->GetResourceByID(resourceID);

						std::optional<GHL::ResourceBarrier> resourceBarrier{ std::nullopt };
						if (isBuffer) {
							resourceBarrier = mResourceStateTracker->TransitionImmediately(resource, expectedStates);
						}
						else {
							resourceBarrier = mResourceStateTracker->TransitionImmediately(resource, subresourceIndex, expectedStates);
						}

						if (resourceBarrier != std::nullopt) {
							resourceBarrierBatch.AddBarrier(*resourceBarrier);
						}
					}
					commandList->D3DCommandList()->ResourceBarrier(resourceBarrierBatch.Size(), resourceBarrierBatch.D3DBarriers());

					auto* currCommandQueue = mCommandQueues.at(queueIndex);
					auto* currFence = mFences.at(queueIndex).get();

					// Wait Command
					for (const auto& waitInfo : transitionNode->waitInfos) {
						if (waitInfo.crossFrame && mFrameTracker->IsFirstFrame()) continue;
						currCommandQueue->WaitFence(*mFences.at(waitInfo.nodeToWait->executionQueueIndex).get());
					}

					// Close And Execute CommandList
					commandList->Close();
					currCommandQueue->ExecuteCommandList(commandList->D3DCommandList());

					// Signal Command
					if (transitionNode->needSignal) {
						currFence->IncrementExpectedValue();
						currCommandQueue->SignalFence(*currFence);
					}
				}

				{
					// 做录制渲染命令操作
					auto commandList = mCommandListAllocator->AllocateCommandList((GHL::EGPUQueue)queueIndex);
					auto& passNodes = currDL->passNodesPerQueue.at(queueIndex);
					auto* currCommandQueue = mCommandQueues.at(queueIndex);
					auto* currFence = mFences.at(queueIndex).get();

					for (auto& passNode : passNodes) {
						// 收集渲染命令
						passNode->renderPass->Execute();

						// Wait Command
						for (const auto& waitInfo : passNode->waitInfos) {
							if (waitInfo.crossFrame && mFrameTracker->IsFirstFrame()) continue;
							currCommandQueue->WaitFence(*mFences.at(waitInfo.nodeToWait->executionQueueIndex).get());
						}

						// Close And Execute CommandList
						commandList->Close();
						currCommandQueue->ExecuteCommandList(commandList->D3DCommandList());

						// Signal Command
						if (passNode->needSignal) {
							currFence->IncrementExpectedValue();
							currCommandQueue->SignalFence(*currFence);
						}
					}
				}
			}
		}
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
			RenderGraphBuilder builder(passNode, mResourceStorage.get());
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

			auto* currPassNode = mPassNodes.at(currIndex);

			// 遍历其他所有的PassNode
			for (size_t otherIndex = currIndex; otherIndex < mPassNodes.size(); otherIndex++) {
				if (currIndex == otherIndex) continue;

				auto* otherPassNode = mPassNodes.at(otherIndex);

				// 遍历otherPassNode的所有读资源，判断它是否依赖于currPassNode的写资源
				for (const auto& readSubresourceID : otherPassNode->readSubresources) {

					// 发现资源依赖
					if (currPassNode->writeSubresources.find(readSubresourceID) != currPassNode->writeSubresources.end()) {
						// 存储邻接表
						mAdjacencyLists.at(currIndex).push_back(otherIndex);
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

			for (uint8_t queueIndex = 0; queueIndex < std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Count); queueIndex++) {
				mGraphNodes.emplace_back(std::make_unique<TransitionNode>());
				mTransitionNodes.push_back(static_cast<TransitionNode*>(mGraphNodes.back().get()));
				auto* transitionNode = mTransitionNodes.back();
				transitionNode->executionQueueIndex = queueIndex;
				transitionNode->graphNodeIndex = mGraphNodes.size() - 1u;
				transitionNode->transitionNodeIndex = mTransitionNodes.size() - 1u;
				transitionNode->dependencyLevelIndex = i;
				
				mDependencyLevelList.back()->transitionNodePerQueue.at(queueIndex) = transitionNode;
			}
		}

		for (size_t i = 0u; i < mSortedPassNodes.size(); i++) {
			uint64_t passNodeIndex = mSortedPassNodes.at(i);

			auto* passNode = mPassNodes.at(passNodeIndex);
			uint64_t passDependencyLevel = longestDistances.at(passNodeIndex);

			auto& dependencyLevel = mDependencyLevelList.at(passDependencyLevel);
			dependencyLevel->passNodesPerQueue.at(passNode->executionQueueIndex).push_back(passNode);

			passNode->dependencyLevelIndex = passDependencyLevel;
		}
	}

	void RenderGraph::BuildRenderGraphEdge() {
		// 构建由顺序执行产生的GraphEdge
		{
			std::vector<std::vector<GraphNode*>> graphNodesPerQueue;
			graphNodesPerQueue.resize(std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Count));
			for (size_t i = 0; i < mDependencyLevelList.size(); i++) {
				auto* currDL = mDependencyLevelList.at(i).get();
				auto* lastDL = i == 0u ? nullptr : mDependencyLevelList.at(i - 1u).get();

				for (uint8_t queueIndex = 0u; queueIndex < std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Count); queueIndex++) {
					if (currDL->passNodesPerQueue.at(queueIndex).empty()) {
						continue;
					}
					
					// Add Transiton Node
					auto* transitionNode = currDL->transitionNodePerQueue.at(queueIndex);
					if (graphNodesPerQueue.at(queueIndex).size() > 0u) {
						auto* backGraphNode = graphNodesPerQueue.at(queueIndex).back();
						mGraphEdges.emplace_back(backGraphNode->graphNodeIndex, transitionNode->graphNodeIndex, true, false);
					}
					graphNodesPerQueue.at(queueIndex).push_back(transitionNode);

					// Add Pass Node
					for (const auto& passNode : currDL->passNodesPerQueue.at(queueIndex)) {
						auto* backGraphNode = graphNodesPerQueue.at(queueIndex).back();
						mGraphEdges.emplace_back(backGraphNode->graphNodeIndex, passNode->graphNodeIndex, true, false);
						graphNodesPerQueue.at(queueIndex).push_back(passNode);
					}
				}
			}
		}

		// 构建由读写依赖产生的GraphEdge
		for (size_t i = 0; i < mDependencyLevelList.size(); i++) {
			GHL::EResourceState currExpectedStates{ GHL::EResourceState::Common };
			GHL::EResourceState lastExpectedStates{ GHL::EResourceState::Common };
			std::unordered_set<uint8_t> currRequiredQueues;
			std::unordered_set<uint8_t> lastRequiredQueues;
			DependencyLevel* currDL = mDependencyLevelList.at(i).get();
			DependencyLevel* lastDL = nullptr;

			// 获取子资源在某个DL中被需求的信息
			auto GetSubresourceRequestion = [](const DependencyLevel* currDl, const SubresourceID& subresourceID, GHL::EResourceState& expectedStates, std::unordered_set<uint8_t>& requiredQueues) {
			
			};

			for (const auto& readSubresourceID : currDL->readSubresources) {

				GetSubresourceRequestion(currDL, readSubresourceID, currExpectedStates, currRequiredQueues);
				bool crossFrame{ false }; // 读写依赖是否跨帧

				int32_t lastDlIndex = i - 1;
				while (lastDlIndex != i) {
					if (lastDlIndex < 0) {
						lastDlIndex = mDependencyLevelList.size() - 1;
						crossFrame = true;
					}

					lastDL = mDependencyLevelList.at(lastDlIndex).get();
					GetSubresourceRequestion(lastDL, readSubresourceID, lastExpectedStates, lastRequiredQueues);
					if (!lastRequiredQueues.empty()) break;
				}
				if (lastRequiredQueues.empty()) {
					ASSERT_FORMAT(false, "Subresource Never Existed!");
				}

				if (currExpectedStates == lastExpectedStates) continue;

				// 获取可以支持资源进行状态转换的队列索引
				uint8_t targetQueueIndex = *currRequiredQueues.begin();
				while (targetQueueIndex >= 0) {
					if (targetQueueIndex >= std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Count)) {
						ASSERT_FORMAT(false, "Queue Index Out Of Range!");
					}

					bool isSupport = GHL::IsStatesSupportedOnQueue(lastExpectedStates, (GHL::EGPUQueue)targetQueueIndex) &&
						GHL::IsStatesSupportedOnQueue(currExpectedStates, (GHL::EGPUQueue)targetQueueIndex);

					if (isSupport) break;

					targetQueueIndex--;
				}

				auto* targetTransitionNode = currDL->transitionNodePerQueue.at(targetQueueIndex);
				targetTransitionNode->expectedSubresourceStatesMap[readSubresourceID] = currExpectedStates;
				for (const auto& queueIndex : lastRequiredQueues) {
					if (queueIndex == targetQueueIndex) continue;
					auto* lastPassNode = lastDL->passNodesPerQueue.at(queueIndex).back();
					mGraphEdges.emplace_back(lastPassNode->graphNodeIndex, targetTransitionNode->graphNodeIndex, lastPassNode->executionQueueIndex != targetTransitionNode->executionQueueIndex, crossFrame);
				}

				for (const auto& queueIndex : currRequiredQueues) {
					if (queueIndex == targetQueueIndex) continue;
					auto* nextPassNode = currDL->passNodesPerQueue.at(queueIndex).front();
					mGraphEdges.emplace_back(targetTransitionNode->graphNodeIndex, nextPassNode->graphNodeIndex, nextPassNode->executionQueueIndex != targetTransitionNode->executionQueueIndex, false);
				}
			}

			for (const auto& writeSubresourceID : currDL->writeSubresources) {
				GetSubresourceRequestion(currDL, writeSubresourceID, currExpectedStates, currRequiredQueues);

				bool crossFrame{ false }; // 读写依赖是否跨帧

				int32_t lastDlIndex = i - 1;
				while (lastDlIndex != i) {
					if (lastDlIndex < 0) {
						lastDlIndex = mDependencyLevelList.size() - 1;
						crossFrame = true;
					}

					lastDL = mDependencyLevelList.at(lastDlIndex).get();
					GetSubresourceRequestion(lastDL, writeSubresourceID, lastExpectedStates, lastRequiredQueues);
					if (!lastRequiredQueues.empty()) break;
				}
				if (lastRequiredQueues.empty()) {
					ASSERT_FORMAT(false, "Subresource Never Used!");
				}

				uint8_t targetQueueIndex = *currRequiredQueues.begin();
				while (targetQueueIndex >= 0) {
					if (targetQueueIndex >= std::underlying_type<GHL::EGPUQueue>::type(GHL::EGPUQueue::Count)) {
						ASSERT_FORMAT(false, "Queue Index Out Of Range!");
					}

					bool isSupport = GHL::IsStatesSupportedOnQueue(lastExpectedStates, (GHL::EGPUQueue)targetQueueIndex) &&
						GHL::IsStatesSupportedOnQueue(currExpectedStates, (GHL::EGPUQueue)targetQueueIndex);

					if (isSupport) break;

					targetQueueIndex--;
				}

				auto* targetTransitionNode = currDL->transitionNodePerQueue.at(targetQueueIndex);
				targetTransitionNode->expectedSubresourceStatesMap[writeSubresourceID] = currExpectedStates;
				for (const auto& queueIndex : lastRequiredQueues) {
					if (queueIndex == targetQueueIndex || crossFrame) continue;
					auto* lastPassNode = lastDL->passNodesPerQueue.at(queueIndex).back();
					mGraphEdges.emplace_back(lastPassNode->graphNodeIndex, targetTransitionNode->graphNodeIndex, lastPassNode->executionQueueIndex != targetTransitionNode->executionQueueIndex, crossFrame);
				}

				for (const auto& queueIndex : currRequiredQueues) {
					if (queueIndex == targetQueueIndex) continue;
					auto* nextPassNode = currDL->passNodesPerQueue.at(queueIndex).front();
					mGraphEdges.emplace_back(targetTransitionNode->graphNodeIndex, nextPassNode->graphNodeIndex, nextPassNode->executionQueueIndex != targetTransitionNode->executionQueueIndex, false);
				}
			}

		}

	}

	void RenderGraph::CullRedundantGraphEdge() {
		// 使用节点与依赖边与弗洛伊德算法做冗余剔除
		uint64_t nodeSize = mGraphNodes.size();

		std::vector<std::vector<int64_t>> dist(nodeSize, std::vector<int64_t>(nodeSize, INFINITE));

		for (uint64_t i = 0u; i < nodeSize; i++) {
			dist[i][i] = 0;
		}

		for (const auto& edge : mGraphEdges) {
			if (edge.crossFrame) continue;
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
			auto* producerGraphNode = mGraphNodes.at(edge.producerNodeIndex).get();
			auto* consumerGraphNode = mGraphNodes.at(edge.consumerNodeIndex).get();

			producerGraphNode->needSignal = true;
			consumerGraphNode->waitInfos.emplace_back(edge.crossFrame, producerGraphNode);
		}
	}

	void RenderGraph::BuildAliasingBarrier() {
		mResourceStorage->BuildAliasing();
	}

}