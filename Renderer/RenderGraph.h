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
		void ImportResource(const std::string& name, Buffer* importedBuffer);

		/*
		* ���ⲿ����������Դ
		*/
		void ImportResource(const std::string& name, Texture* importedTexture);

	private:

		/*
		* Set up Internal Pipeline Resource
		*/
		void SetupInternalResource();

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
		* ����ͼ�ı߼�
		*/
		void BuildRenderGraphEdge();

		/*
		* �޳�����������
		*/
		void CullRedundantGraphEdge();

		/*
		* ������Դ��������
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

		std::vector<uint64_t> mSortedPassNodes; // ���������Ľ��
		
		std::vector<std::vector<uint64_t>> mAdjacencyLists; // PassNodes���ڽӱ�

		std::vector<std::unique_ptr<DependencyLevel>> mDependencyLevelList; // �����㼶

		std::unique_ptr<RenderGraphResourceStorage> mResourceStorage; // �洢������Դ

		std::unique_ptr<RenderGraphResourceStateTracker> mResourceStateTracker;
	};

}

#include "RenderGraph.inl"