#pragma once

#include "RingFrameTracker.h"
#include "RenderGraphResource.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <stack>
#include <string>

namespace Renderer {

	class RenderGraphPass;

	/*
	* Pass的目标GPU队列
	*/
	enum class PassExecutionQueue : uint8_t {
		General = 0, // 通用的图形引擎
		Compute = 1, // 异步计算引擎
		Copy    = 2, // 异步复制引擎
		Count   = 3
	};

	class RenderGraph {
	public:

		struct PassNode {
		public:
			/*
			* 设置资源的期望状态，由RenderGraphBuilder调用
			*/
			void SetExpectedState(const std::string& name, GHL::EResourceState expectedState);

			/*
			* 添加读依赖
			*/
			void AddReadDependency(const std::string& name);

			/*
			* 添加写依赖
			*/
			void AddWriteDependency(const std::string& name);

			/*
			* 设定PassNode所属的GPU引擎队列
			*/
			void SetExecutionQueue(PassExecutionQueue queueIndex);

		public:
			RenderGraphPass* pass{ nullptr };    // 该PassNode的内部执行方法
			uint8_t executionQueueIndex;

			std::unordered_map<std::string, GHL::EResourceState> expectedStateMap; // 该PassNode所使用的资源，及其期望状态

			// 该PassNode的读写依赖，用于构造有向图

			std::unordered_set<std::string> readDependency;
			std::unordered_set<std::string> writeDependency;

			// 以下参数在RenderGraph构造时写入

			uint64_t globalExecutionIndex{ 0u }; // 该PassNode的全局执行顺序
			uint64_t dependencyLevelIndex{ 0u }; // 该PassNode所在的依赖级别
			uint64_t localToDependencyLevelExecutionIndex{ 0u };  // 该PassNode在DependencyLevel内的顺序
			uint64_t localToQueueExecutionIndex{ 0u }; // 该PassNode在其对应的Queue上的顺序

			bool syncSignalRequired{ false }; // 需要设置同步信号，以通知其他同步等待的PassNode

			std::vector<PassNode*> nodesToSyncWith; // 执行该PassNode前需要同步等待的其他PassNode
		};

		class DependencyLevel {
		public:
			DependencyLevel();
			~DependencyLevel() = default;

			void SetLevel(uint64_t level);

			void AddNode(PassNode* passNode);

			inline auto&       GetPassNodes()       { return mPassNodes; }
			inline const auto& GetPassNodes() const { return mPassNodes; }
			inline const auto& GetNodeSize() const  { return mPassNodes.size(); }

		private:
			uint64_t mLevelIndex{ 0u };
			std::vector<PassNode*> mPassNodes;
			std::vector<std::vector<PassNode*>> mPassNodesPerQueue; // 每一个GPU队列上的Pass
		};

	public:
		RenderGraph(RingFrameTracker* frameTracker);
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
		* 构建邻接表
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
		* 遍历所有的依赖层级，计算资源的生命周期，构建SSIS
		*/
		void TraverseDependencyLevels();

	private:
		RingFrameTracker* mFrameTracker{ nullptr };

		bool mBuilded{ false };
		std::vector<std::unique_ptr<RenderGraphPass>> mRenderGraphPasses;
		std::vector<std::unique_ptr<PassNode>> mPassNodes;
		std::vector<PassNode*> mSortedPassNodes; // 拓扑排序后的PassNodes

		std::unordered_map<std::string, std::unique_ptr<RenderGraphResource>> mResources;
		std::unordered_map<std::string, RenderGraphResource*> mImportedResources;
		
		std::vector<std::vector<uint64_t>> mAdjacencyLists; // PassNode的邻接表
		std::vector<DependencyLevel> mDependencyList;
		std::vector<uint64_t> mPassCountPerQueue; // 记录每一个Queue上需要执行的Pass个数
	};

}

#include "RenderGraph.inl"