#pragma once
#include "GHL/pbh.h"
#include "GHL/ResourceBarrierBatch.h"

#include "RenderGraphResourceID.h"

#include <unordered_map>
#include <unordered_set>
#include <optional>

namespace Renderer {

	class RenderGraphPass;
	class GraphNode;

	struct WaitInfo {
	public:
		WaitInfo(bool crossFrame, GraphNode* node) 
		: crossFrame(crossFrame), nodeToWait(node) {}

	public:
		bool crossFrame{ false }; // 该等待是否跨帧
		GraphNode* nodeToWait{ nullptr };
	};

	/*
	* Edge In RenderGraph
	*/
	struct GraphEdge {
	public:
		GraphEdge(uint64_t proNode, uint64_t conNode, bool crossQueue, bool crossFrame) 
		: producerNodeIndex(proNode), consumerNodeIndex(conNode), crossQueue(crossQueue), crossFrame(crossFrame) {}

	public:
		uint64_t producerNodeIndex{ 0u };
		uint64_t consumerNodeIndex{ 0u };
		bool crossQueue{ false };
		bool crossFrame{ false };
	};

	struct GraphNode {
	public:
		virtual ~GraphNode() = default;
	public:
		uint8_t executionQueueIndex{ 0u };
		uint8_t graphNodeIndex{ 0u };

		bool needSignal{ false };
		std::vector<WaitInfo> waitInfos;
	};

	/*
	* Pass Node In RenderGraph
	*/
	struct PassNode : GraphNode {
	public:
		void AddReadDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceIndex);
		void AddReadDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceStartIndex, uint32_t subresourceCount);
		void AddReadDependency(const RenderGraphResourceID& resourceID, std::vector<uint32_t>&& subresourceIndexList);

		void AddWriteDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceIndex);
		void AddWriteDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceStartIndex, uint32_t subresourceCount);
		void AddWriteDependency(const RenderGraphResourceID& resourceID, std::vector<uint32_t>&& subresourceIndexList);

		void SetExecutionQueue(GHL::EGPUQueue queue = GHL::EGPUQueue::Graphics);

	public:
		RenderGraphPass* renderPass{ nullptr };
		std::unordered_set<SubresourceID> readSubresources;
		std::unordered_set<SubresourceID> writeSubresources;

		uint64_t passNodeIndex{ 0u };
		uint64_t dependencyLevelIndex{ 0u };

		std::unordered_set<SubresourceID> aliasedSubresources;
	};

	/*
	* 资源状态转换节点
	*/
	struct TransitionNode : GraphNode {
	public:
		uint64_t transitionNodeIndex{ 0u };
		uint64_t dependencyLevelIndex{ 0u };

		std::unordered_map<SubresourceID, GHL::EResourceState> expectedSubresourceStatesMap;
	};

	/*
	* Dependency Level
	*/
	struct DependencyLevel {
	public:
		DependencyLevel();
		~DependencyLevel() = default;

	public:
		uint64_t levelIndex{ 0u };

		std::unordered_set<SubresourceID> readSubresources;
		std::unordered_set<SubresourceID> writeSubresources;

		std::vector<TransitionNode*> transitionNodePerQueue;
		std::vector<std::vector<PassNode*>> passNodesPerQueue;
	};

}