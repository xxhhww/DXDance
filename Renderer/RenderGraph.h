#pragma once
#include "RenderGraphItem.h"
#include "RenderGraphPass.h"
#include "ResourceStateTracker.h"
#include "RenderGraphResourceStorage.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <set>
#include <stack>
#include <string>

namespace GHL {

	class Device;
	class Fence;
	class CommandQueue;
	class GraphicsQueue;
	class ComputeQueue;
	class CopyQueue;

}

namespace Renderer {

	class Buffer;
	class Texture;
	class RenderGraphPass;
	class RingFrameTracker;
	class PoolDescriptorAllocator;
	class PoolCommandListAllocator;

	class RenderGraph {
	public:
		RenderGraph(const GHL::Device* device, RingFrameTracker* frameTracker, PoolDescriptorAllocator* descriptorAllocator);
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
		void ImportResource(const std::string& name, Buffer* importedBuffer);

		/*
		* 从外部导入纹理资源
		*/
		void ImportResource(const std::string& name, Texture* importedTexture);

	private:

		/*
		* Set up Internal Pipeline Resource
		*/
		void SetupInternalResource();

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
		* 构建图的边集
		*/
		void BuildRenderGraphEdge();

		/*
		* 剔除冗余依赖边
		*/
		void CullRedundantGraphEdge();

		/*
		* 构建资源别名屏障
		*/
		void BuildAliasingBarrier();

	private:
		RingFrameTracker* mFrameTracker{ nullptr };
		PoolCommandListAllocator* mCommandListAllocator{ nullptr };

		std::vector<GHL::CommandQueue*> mCommandQueues;
		std::vector<std::unique_ptr<GHL::Fence>> mFences;

		bool mCompiled{ false };

		std::vector<std::unique_ptr<RenderGraphPass>> mRenderGraphPasses;

		std::vector<GraphEdge> mGraphEdges;
		std::vector<std::unique_ptr<GraphNode>> mGraphNodes;
		std::vector<PassNode*> mPassNodes;
		std::vector<TransitionNode*> mTransitionNodes;

		std::vector<uint64_t> mSortedPassNodes; // 拓扑排序后的结果
		
		std::vector<std::vector<uint64_t>> mAdjacencyLists; // PassNodes的邻接表

		std::vector<std::unique_ptr<DependencyLevel>> mDependencyLevelList; // 依赖层级

		std::unique_ptr<RenderGraphResourceStorage> mResourceStorage; // 存储管线资源

		std::unique_ptr<RenderGraphResourceStateTracker> mResourceStateTracker;
	};

}

#include "RenderGraph.inl"