#pragma once

#include "RingFrameTracker.h"
#include "RenderGraphPass.h"
#include "RenderGraphResourceStorage.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <string>

namespace Renderer {
	/*
	* Pass��Ŀ��GPU����
	*/
	enum class PassExecutionQueue : uint8_t {
		General = 0, // ͨ�õ�ͼ������
		Compute = 1, // �첽��������
		Copy    = 2, // �첽��������
		Count
	};

	class RenderGraph {
	public:

		/*
		* Graph�еĶ���
		*/
		struct GraphNode {
		public:
			/*
			* ��Ӷ�����
			*/
			void AddReadDependency(const std::string& name);

			/*
			* ���д����
			*/
			void AddWriteDependency(const std::string& name);

			/*
			* �趨����Դ��״̬����
			*/
			void SetExpectedStates(const std::string& name, GHL::EResourceState state);

			/*
			* �趨PassNode������GPU�������
			*/
			void SetExecutionQueue(PassExecutionQueue queueIndex);

		public:
			RenderGraphPass* pass{ nullptr };
			uint8_t executionQueueIndex;

			std::unordered_map<std::string, GHL::EResourceState> expectedStatesMap; // �ýڵ����Դ���ڴ�״̬

			// �ڵ�֮��Ķ�д���������ڹ���GraphEdge��DAG

			std::unordered_set<std::string> readDependency;
			std::unordered_set<std::string> writeDependency;

			uint64_t nodeIndex{ 0u };
			uint64_t globalExecutionIndex{ 0u };
			uint64_t dependencyLevelIndex{ 0u };
			uint64_t localToQueueExecutionIndex{ 0u };

			bool requireSyncSignal{ false }; // �ýڵ��Ƿ���Ҫ����Signal������֪ͨ�ȴ��ýڵ�������ڵ�(һ���ǿ���еĽڵ�)

			std::vector<GraphNode*> nodesToSyncWait; // �ýڵ�����ǰ��Ҫͬ���ȴ��������ڵ�
		};

		/*
		* Graph�еı�
		*/
		struct GraphEdge {
		public:
			GraphEdge(uint64_t producerNodeIndex, uint64_t consumerNodeIndex, bool crossQueue);
			~GraphEdge() = default;

			uint64_t producerNodeIndex{ 0u }; // �ñߵ������߽ڵ������
			uint64_t consumerNodeIndex{ 0u }; // �ñߵ������߽ڵ������
			bool crossQueue{ false };         // �ñ��Ƿ�����
		};

		/*
		* �����㼶
		*/
		class DependencyLevel {
		public:
			DependencyLevel();
			~DependencyLevel() = default;

			void SetLevel(uint64_t level);

			void AddNode(GraphNode* passNode);

			inline auto&       GetGraphNodes()       { return mGraphNodes; }
			inline const auto& GetGraphNodes() const { return mGraphNodes; }
			inline const auto& GetNodeSize()   const { return mGraphNodes.size(); }

		private:
			uint64_t mLevelIndex{ 0u };
			std::vector<GraphNode*> mGraphNodes;
			std::vector<std::vector<GraphNode*>> mGraphNodesPerQueue; // ÿһ��GPU�����ϵ�Pass
		};

	public:
		RenderGraph(const GHL::Device* device, RingFrameTracker* frameTracker);
		~RenderGraph() = default;

		/*
		* ���RenderPass
		*/
		template<typename ...Args>
		void AddPass(Args&&... args);

		/*
		* ������Ⱦ����ͼ
		*/
		void Build();

		/*
		* ִ����Ⱦ����
		*/
		void Execute();

		/*
		* ���ⲿ���뻺����Դ
		*/
		void ImportResource(Buffer* importedBuffer);

		/*
		* ���ⲿ����������Դ
		*/
		void ImportResource(Texture* importedTexture);

	private:

		/*
		* �����ڽӱ����������Դ��д����������GraphEdge
		*/
		void BuildAdjacencyList();

		/*
		* ���ڽӱ�����������
		*/
		void TopologicalSort();

		/*
		* ���ڽӱ��ɵ�����ͼ���������������
		*/
		void DepthFirstSearch(uint64_t nodeIndex, std::vector<bool>& visited, std::stack<uint64_t>& stack);

		/*
		* ���������㼶
		*/
		void BuildDependencyLevels();

		/*
		* �޳���������
		*/
		void CullRedundantDependencies();

	private:
		RingFrameTracker* mFrameTracker{ nullptr };

		bool mCompiled{ false };
		std::vector<std::unique_ptr<RenderGraphPass>> mRenderGraphPasses;
		
		std::vector<std::unique_ptr<GraphNode>> mGraphNodes;
		std::vector<GraphEdge> mGraphEdges;

		std::vector<uint64_t> mSortedGraphNodes; // ���������Ľ��
		
		std::vector<std::vector<uint64_t>> mAdjacencyLists; // GraphNodes���ڽӱ�

		std::vector<DependencyLevel> mDependencyList; // �����㼶
		std::vector<std::vector<uint64_t>> mGraphNodesPerQueue;

		std::unique_ptr<RenderGraphResourceStorage> mResourceStorage; // �洢������Դ
	};

}

#include "RenderGraph.inl"