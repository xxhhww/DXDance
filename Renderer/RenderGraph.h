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
	class Display;
}

namespace Renderer {

	class Resource;
	class RenderGraphPass;
	class RingFrameTracker;
	class PoolDescriptorAllocator;
	class PoolCommandListAllocator;
	class ShaderManger;
	class CommandSignatureManger;
	class LinearBufferAllocator;

	struct RenderContext {
	public:
		RenderContext(
			const GHL::Display* _display,
			const GHL::Device* _device,
			ShaderManger* _shaderManger, 
			CommandSignatureManger* _commandSignatureManger,
			LinearBufferAllocator* _dynamicAllocator, 
			RenderGraphResourceStorage* _resourceStorage,
			ResourceStateTracker* _resourceStateTracker,
			RingFrameTracker* _ringFrameTracker)
		: display(_display)
		, device(_device)
		, shaderManger(_shaderManger)
		, commandSignatureManger(_commandSignatureManger)
		, dynamicAllocator(_dynamicAllocator)
		, resourceStorage(_resourceStorage)
		, resourceStateTracker(_resourceStateTracker)
		, frameTracker(_ringFrameTracker) {}

		const GHL::Display* display{ nullptr };
		const GHL::Device*  device{ nullptr };

		ShaderManger* shaderManger{ nullptr };
		CommandSignatureManger* commandSignatureManger{ nullptr };
		LinearBufferAllocator* dynamicAllocator{ nullptr };
		RenderGraphResourceStorage* resourceStorage{ nullptr };
		ResourceStateTracker* resourceStateTracker{ nullptr };
		RingFrameTracker* frameTracker{ nullptr };
	};

	class RenderGraph {
	public:
		RenderGraph(
			const GHL::Display* display, // For Tone Mapping Pass
			const GHL::Device* device,
			RingFrameTracker* frameTracker,
			PoolDescriptorAllocator* descriptorAllocator,
			PoolCommandListAllocator* commandListAllocator,
			GHL::GraphicsQueue* graphicsQueue,
			GHL::ComputeQueue* computeQueue,
			GHL::CopyQueue* copyQueue,
			ResourceStateTracker* stateTracker,
			ShaderManger* shaderManger,
			CommandSignatureManger* commandSignatureManger,
			LinearBufferAllocator* dynamicAllocator);
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
		* 从外部导入资源
		*/
		RenderGraphResourceID ImportResource(const std::string& name, Resource* importedBuffer);

		/*
		* 删除外部导入的资源
		*/
		void ExportResource(const std::string& name);

		inline auto* GetPipelineResourceStorage() const { return mResourceStorage.get(); }

	private:

		/*
		* Set up Internal Pipeline Resource
		*/
		void SetupInternalResource();

		/*
		* Start Tracking Internal Resource
		*/
		void StartTrackingInternalResource();

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
		const GHL::Display* mDisplay{ nullptr }; // For Tone Mapping
		const GHL::Device*  mDevice{ nullptr };

		RingFrameTracker* mFrameTracker{ nullptr };
		PoolDescriptorAllocator*  mDescriptorAllocator{ nullptr };
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

		ResourceStateTracker* mResourceStateTracker{ nullptr };
		ShaderManger* mShaderManger{ nullptr };
		CommandSignatureManger* mCommandSignatureManger{ nullptr };
		LinearBufferAllocator* mDynamicAllocator{ nullptr };
	};

}

#include "RenderGraph.inl"