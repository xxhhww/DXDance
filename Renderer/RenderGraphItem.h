#pragma once
#include "GHL/ResourceBarrierBatch.h"
#include "RenderGraphResourceID.h"
#include <unordered_set>
#include <optional>

namespace Renderer {

	class RenderGraphPass;
	class BarrierNode;

	/*
	* Pass��Ŀ��GPU����
	*/
	enum class PassExecutionQueue : uint8_t {
		General = 0,	// ͨ�õ�ͼ������
		Compute = 1,	// �첽��������
		Copy = 2,		// �첽��������
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

		GHL::ResourceBarrierBatch aliasingBarrierBatch; // ��Դ��������

		std::vector<BarrierNode*> barrierNodesToWait; // PassNodeִ��ǰ��Ҫ����еȴ���BarrierNode
	};

	/*
	* ��Դ���Ͻڵ㣬������DL֮��
	*/
	struct BarrierNode {
	public:
		bool isRerouted{ false };	// �����Ƿ����²���
		std::optional<uint8_t> reroutedIndex{ std::nullopt };	// ������Ͻڵ㱻���²��ߣ����ֵ���Ŀ�����Ͻڵ������

		uint8_t executionQueueIndex{ 0u };
		GHL::ResourceBarrierBatch generalBarrierBatch; // ��Դ����

		std::vector<PassNode*> passNodesNeedToWait; // BarrierNodeִ��ǰ��Ҫ����еȴ���PassNode
		bool needSignal{ false }; // �Ƿ���Ҫִ��Signal����
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