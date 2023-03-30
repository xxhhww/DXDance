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
	* Pass的目标GPU队列
	*/
	enum class PassExecutionQueue : uint8_t {
		General = 0, // 通用的图形引擎
		Compute = 1, // 异步计算引擎
		Copy    = 2, // 异步复制引擎
		Count
	};

	class RenderGraph {
	public:

		/*
		* Graph中的顶点
		*/
		struct GraphNode {
		public:
			/*
			* 添加读依赖
			*/
			void AddReadDependency(const std::string& name);

			/*
			* 添加写依赖
			*/
			void AddWriteDependency(const std::string& name);

			/*
			* 设定对资源的状态期望
			*/
			void SetExpectedStates(const std::string& name, GHL::EResourceState state);

			/*
			* 设定PassNode所属的GPU引擎队列
			*/
			void SetExecutionQueue(PassExecutionQueue queueIndex);

		public:
			RenderGraphPass* pass{ nullptr };
			uint8_t executionQueueIndex;

			std::unordered_map<std::string, GHL::EResourceState> expectedStatesMap; // 该节点对资源的期待状态

			// 节点之间的读写依赖，用于构造GraphEdge与DAG

			std::unordered_set<std::string> readDependency;
			std::unordered_set<std::string> writeDependency;

			uint64_t nodeIndex{ 0u };
			uint64_t globalExecutionIndex{ 0u };
			uint64_t dependencyLevelIndex{ 0u };
			uint64_t localToQueueExecutionIndex{ 0u };

			bool requireSyncSignal{ false }; // 该节点是否需要发送Signal命令来通知等待该节点的其他节点(一般是跨队列的节点)

			std::vector<GraphNode*> nodesToSyncWait; // 该节点运行前需要同步等待的其他节点
		};

		/*
		* Graph中的边
		*/
		struct GraphEdge {
		public:
			GraphEdge(uint64_t producerNodeIndex, uint64_t consumerNodeIndex, bool crossQueue);
			~GraphEdge() = default;

			uint64_t producerNodeIndex{ 0u }; // 该边的生产者节点的索引
			uint64_t consumerNodeIndex{ 0u }; // 该边的消费者节点的索引
			bool crossQueue{ false };         // 该边是否跨队列
		};

		/*
		* 依赖层级
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
			std::vector<std::vector<GraphNode*>> mGraphNodesPerQueue; // 每一个GPU队列上的Pass
		};

	public:
		RenderGraph(const GHL::Device* device, RingFrameTracker* frameTracker);
		~RenderGraph() = default;

		/*
		* 添加RenderPass
		*/
		template<typename ...Args>
		void AddPass(Args&&... args);

		/*
		* 构建渲染有向图
		*/
		void Build();

		/*
		* 执行渲染任务
		*/
		void Execute();

		/*
		* 从外部导入缓冲资源
		*/
		void ImportResource(Buffer* importedBuffer);

		/*
		* 从外部导入纹理资源
		*/
		void ImportResource(Texture* importedTexture);

	private:

		/*
		* 构建邻接表，并添加由资源读写依赖产生的GraphEdge
		*/
		void BuildAdjacencyList();

		/*
		* 对邻接表做拓扑排序
		*/
		void TopologicalSort();

		/*
		* 对邻接表构成的有向图进行深度优先搜索
		*/
		void DepthFirstSearch(uint64_t nodeIndex, std::vector<bool>& visited, std::stack<uint64_t>& stack);

		/*
		* 构建依赖层级
		*/
		void BuildDependencyLevels();

		/*
		* 剔除冗余依赖
		*/
		void CullRedundantDependencies();

	private:
		RingFrameTracker* mFrameTracker{ nullptr };

		bool mCompiled{ false };
		std::vector<std::unique_ptr<RenderGraphPass>> mRenderGraphPasses;
		
		std::vector<std::unique_ptr<GraphNode>> mGraphNodes;
		std::vector<GraphEdge> mGraphEdges;

		std::vector<uint64_t> mSortedGraphNodes; // 拓扑排序后的结果
		
		std::vector<std::vector<uint64_t>> mAdjacencyLists; // GraphNodes的邻接表

		std::vector<DependencyLevel> mDependencyList; // 依赖层级
		std::vector<std::vector<uint64_t>> mGraphNodesPerQueue;

		std::unique_ptr<RenderGraphResourceStorage> mResourceStorage; // 存储管线资源
	};

}

#include "RenderGraph.inl"