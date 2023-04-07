#pragma once
#include "GHL/ResourceBarrierBatch.h"
#include "RenderGraphResourceID.h"
#include <unordered_set>
#include <optional>

namespace Renderer {

	class RenderGraphPass;
	class BarrierNode;

	/*
	* Pass的目标GPU队列
	*/
	enum class PassExecutionQueue : uint8_t {
		General = 0,	// 通用的图形引擎
		Compute = 1,	// 异步计算引擎
		Copy = 2,		// 异步复制引擎
		Count
	};

	/*
	* Edge In RenderGraph
	*/
	struct GraphEdge {
	public:
		uint64_t producerNodeIndex{ 0u };
		uint64_t consumerNodeIndex{ 0u };
		bool crossQueue{ false };
	};

	/*
	* Pass Node In RenderGraph
	*/
	struct PassNode {
	public:
		
		void AddReadDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceIndex);
		void AddReadDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceStartIndex, uint32_t subresourceCount);
		void AddReadDependency(const RenderGraphResourceID& resourceID, std::vector<uint32_t>&& subresourceIndexList);

		void AddWriteDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceIndex);
		void AddWriteDependency(const RenderGraphResourceID& resourceID, uint32_t subresourceStartIndex, uint32_t subresourceCount);
		void AddWriteDependency(const RenderGraphResourceID& resourceID, std::vector<uint32_t>&& subresourceIndexList);

		void SetExecutionQueue(PassExecutionQueue queue = PassExecutionQueue::General);

	public:
		RenderGraphPass* renderPass{ nullptr };

		std::unordered_set<SubresourceID> readSubresources;
		std::unordered_set<SubresourceID> writeSubresources;

		uint8_t executionQueueIndex{ 0u };

		uint64_t passNodeIndex{ 0u };
		uint64_t dependencyLevelIndex{ 0u };
		uint64_t localToQueueExecutionIndexWithoutBarrier{ 0u };

		GHL::ResourceBarrierBatch aliasingBarrierBatch; // 资源别名屏障

		std::vector<BarrierNode*> barrierNodesToWait; // PassNode执行前需要跨队列等待的BarrierNode
	};

	/*
	* 资源屏障节点，树立在DL之间
	*/
	struct BarrierNode {
	public:
		bool isRerouted{ false };	// 屏障是否被重新布线
		std::optional<uint8_t> reroutedIndex{ std::nullopt };	// 如果屏障节点被重新布线，则该值存放目标屏障节点的索引

		uint8_t executionQueueIndex{ 0u };
		GHL::ResourceBarrierBatch generalBarrierBatch; // 资源屏障

		std::vector<PassNode*> passNodesNeedToWait; // BarrierNode执行前需要跨队列等待的PassNode
		bool needSignal{ false }; // 是否需要执行Signal操作
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

		std::vector<std::vector<BarrierNode*>> barrierNodesPerQueue;

		std::vector<std::vector<PassNode*>> passNodesPerQueue;
		std::vector<PassNode*> passNodes;
	};

}