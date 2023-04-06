#pragma once
#include "GHL/ResourceBarrierBatch.h"
#include "RenderGraphResourceID.h"
#include <unordered_set>
#include <optional>

namespace Renderer {

	class RenderGraphPass;

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

		std::vector<GHL::ResourceBarrierBatch> generalBarrierBatchPerQueue;

		std::vector<std::vector<PassNode*>> passNodesPerQueue;
	};

}