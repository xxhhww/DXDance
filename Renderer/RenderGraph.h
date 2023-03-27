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
	* Pass��Ŀ��GPU����
	*/
	enum class PassExecutionQueue : uint8_t {
		General = 0, // ͨ�õ�ͼ������
		Compute = 1, // �첽��������
		Copy    = 2, // �첽��������
		Count   = 3
	};

	class RenderGraph {
	public:

		struct PassNode {
		public:
			/*
			* ������Դ������״̬����RenderGraphBuilder����
			*/
			void SetExpectedState(const std::string& name, GHL::EResourceState expectedState);

			/*
			* ��Ӷ�����
			*/
			void AddReadDependency(const std::string& name);

			/*
			* ���д����
			*/
			void AddWriteDependency(const std::string& name);

			/*
			* �趨PassNode������GPU�������
			*/
			void SetExecutionQueue(PassExecutionQueue queueIndex);

		public:
			RenderGraphPass* pass{ nullptr };    // ��PassNode���ڲ�ִ�з���
			uint8_t executionQueueIndex;

			std::unordered_map<std::string, GHL::EResourceState> expectedStateMap; // ��PassNode��ʹ�õ���Դ����������״̬

			// ��PassNode�Ķ�д���������ڹ�������ͼ

			std::unordered_set<std::string> readDependency;
			std::unordered_set<std::string> writeDependency;

			// ���²�����RenderGraph����ʱд��

			uint64_t globalExecutionIndex{ 0u }; // ��PassNode��ȫ��ִ��˳��
			uint64_t dependencyLevelIndex{ 0u }; // ��PassNode���ڵ���������
			uint64_t localToDependencyLevelExecutionIndex{ 0u };  // ��PassNode��DependencyLevel�ڵ�˳��
			uint64_t localToQueueExecutionIndex{ 0u }; // ��PassNode�����Ӧ��Queue�ϵ�˳��

			bool syncSignalRequired{ false }; // ��Ҫ����ͬ���źţ���֪ͨ����ͬ���ȴ���PassNode

			std::vector<PassNode*> nodesToSyncWith; // ִ�и�PassNodeǰ��Ҫͬ���ȴ�������PassNode
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
			std::vector<std::vector<PassNode*>> mPassNodesPerQueue; // ÿһ��GPU�����ϵ�Pass
		};

	public:
		RenderGraph(RingFrameTracker* frameTracker);
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
		* �����ڽӱ�
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
		* �������е������㼶��������Դ���������ڣ�����SSIS
		*/
		void TraverseDependencyLevels();

	private:
		RingFrameTracker* mFrameTracker{ nullptr };

		bool mBuilded{ false };
		std::vector<std::unique_ptr<RenderGraphPass>> mRenderGraphPasses;
		std::vector<std::unique_ptr<PassNode>> mPassNodes;
		std::vector<PassNode*> mSortedPassNodes; // ����������PassNodes

		std::unordered_map<std::string, std::unique_ptr<RenderGraphResource>> mResources;
		std::unordered_map<std::string, RenderGraphResource*> mImportedResources;
		
		std::vector<std::vector<uint64_t>> mAdjacencyLists; // PassNode���ڽӱ�
		std::vector<DependencyLevel> mDependencyList;
		std::vector<uint64_t> mPassCountPerQueue; // ��¼ÿһ��Queue����Ҫִ�е�Pass����
	};

}

#include "RenderGraph.inl"