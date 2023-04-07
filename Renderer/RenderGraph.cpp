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

			// �����������е�PassNode
			for (size_t otherIndex = 0; otherIndex < mPassNodes.size(); otherIndex++) {
				if (currIndex == otherIndex) continue;

				auto& otherPassNode = mPassNodes.at(otherIndex);

				// ����otherPassNode�����ж���Դ���ж����Ƿ�������currPassNode��д��Դ
				for (const auto& readSubresourceID : otherPassNode->readSubresources) {

					// ������Դ����
					if (currPassNode->writeSubresources.find(readSubresourceID) != currPassNode->writeSubresources.end()) {
						// �洢�ڽӱ�
						mAdjacencyLists.at(currIndex).push_back(otherIndex);
						
						// �洢GraphEdge
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
			// ��GraphEdge���������Queue�ϵ�ִ��˳�����γɵ�����
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
			// ������Դ����������
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
		// ʹ�ýڵ����������븥�������㷨�������޳�
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

		// ɾ������ı�
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

		// ����еı���Ҫ�����¼
		for (const auto& edge : mGraphEdges) {
			if (!edge.crossQueue) {
				continue;
			}

			// ��û�б��޳������Ҹñ߿����
			auto& producerGraphNode = mGraphNodes.at(edge.producerNodeIndex);
			auto& consumerGraphNode = mGraphNodes.at(edge.consumerNodeIndex);

			producerGraphNode->requireSyncSignal = true;
			consumerGraphNode->nodesToSyncWait.push_back(producerGraphNode.get());
		}
		*/
	}

	void RenderGraph::BuildTransitionBarrier() {
		
		// ת�������ز��ߵļ������:
		// 1. Ŀ����Դ�ڵ�ǰDL�д��ڿ���ж�ȡ
		// 2. ����Դת�����ϵ�ǰ��״̬����Ŀ�������֧��(����: ������в�֧��PixelShaderAccess)
		for (size_t i = 0; i < mDependencyLevelList.size(); i++) {
			auto* currDL = mDependencyLevelList.at(i).get();
			auto* lastDL = (i == 0) ? nullptr : mDependencyLevelList.at(i - 1u).get();

			std::unordered_set<SubresourceID> visitedSubresourceIDs;

			for (auto* currPassNode : currDL->passNodes) {

				// process read subresource
				for (auto& readSubresourceID : currPassNode->readSubresources) {
					if (visitedSubresourceIDs.find(readSubresourceID) != visitedSubresourceIDs.end()) {
						// ��ǰDL�ڣ��Ѿ��������ڵ���ʹ�������Դ��
						visitedSubresourceIDs.insert(readSubresourceID);
						continue;
					}

					// ����SubresourceID����ö�Ӧ��RenderGraphResource
					auto [resourceID, subresourceIndex, isBuffer] = DecodeSubresourceID(readSubresourceID);
					auto* resource = mResourceStorage->GetResourceByID(resourceID);

					// ��Resource����Ķ���
					std::unordered_set<uint8_t> queuesRequiredForResource;
					queuesRequiredForResource.insert(currPassNode->executionQueueIndex);

					GHL::EResourceState expectedStatesInCurrDL{ GHL::EResourceState::Common };
					bool readByMutipleQueues{ false };
					bool needReroutingOnAllQueues{ false };

					expectedStatesInCurrDL = resource->GetSubresourceRequestedInfo(currPassNode->passNodeIndex, subresourceIndex);

					// �ռ�CurrDL����������PassNode�Ը���Դ������
					for (auto* otherPassNode : currDL->passNodes) {
						if (currPassNode == otherPassNode) {
							continue;
						}

						// ��DL�е�����PassNode�뵱ǰ��PassNode�����˶�д��ͻ
						ASSERT_FORMAT(otherPassNode->writeSubresources.find(readSubresourceID) == otherPassNode->writeSubresources.end(),
							"A Read Write Conflict Has Occurred");

						if (otherPassNode->readSubresources.find(readSubresourceID) == otherPassNode->readSubresources.end()) {
							continue;
						}

						// otherPassNodeҲ��ȡ��Resource

						expectedStatesInCurrDL |= resource->GetSubresourceRequestedInfo(otherPassNode->passNodeIndex, subresourceIndex);
						
						if (queuesRequiredForResource.find(otherPassNode->executionQueueIndex) != queuesRequiredForResource.end()) {
							continue;
						}

						queuesRequiredForResource.insert(otherPassNode->executionQueueIndex);
						readByMutipleQueues = true;
					}
 
					std::optional<GHL::ResourceBarrier> barrier = mResourceStateTracker->TransitionImmediately(resource, expectedStatesInCurrDL, true);
					if (!barrier) {
						// ����Դ��CurrDL��״̬��LastDL����
						continue;
					}

					if (readByMutipleQueues) {
						// ��Դ������ж�ȡ�����һ���Ƿ����һ������֧����Դ��ǰ��״̬
						std::optional<uint8_t> availableQueueIndex{ std::nullopt };
						availableQueueIndex = FindClosestQueue();


						if (!availableQueueIndex) {
							// ����Щ���ж�������ʱ����Ҫ���и���Χ���ز���
							needReroutingOnAllQueues = true;
						}
						else {
							// ������Щ�����ڲ������ز���
						}
					}
					else {
						// ��Դ����һ�����ж�ȡ

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