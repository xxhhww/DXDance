#pragma once

#include "RingFrameTracker.h"
#include "RenderGraphResource.h"

#include <memory>
#include <unordered_map>
#include <unordered_set>
#include <string>

namespace Renderer {

	class RenderGraphPass;

	/*
	* Pass��Ŀ��GPU����
	*/
	enum class PassExecutionQueue : uint8_t {
		General = 0, // ͨ�õ�ͼ������
		Compute = 1, // �첽��������
		Copy    = 2  // �첽��������
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
			* ��Ӷ�д����
			*/

			/*
			* �趨PassNode������GPU����
			*/
			void SetExecutionQueue(PassExecutionQueue queueIndex);

		public:
			RenderGraphPass* pass{ nullptr };    // ��PassNode���ڲ�ִ�з���
			PassExecutionQueue executionQueueIndex;

			std::unordered_map<std::string, GHL::EResourceState> expectedStateMap; // ��PassNode��ʹ�õ���Դ����������״̬

			// ��PassNode�Ķ�д���������ڹ�������ͼ

			std::unordered_set<std::string> readDependency;
			std::unordered_set<std::string> writeDependency;

			// ���²����ڹ�������ͼʱд��

			uint64_t globalExecutionIndex{ 0u }; // ��PassNode��ȫ��ִ��˳��
			uint64_t dependencyLevelIndex{ 0u }; // ��PassNode���ڵ���������
			uint64_t localExecutionIndex{ 0u };  // ��PassNode�ľֲ�ִ��˳��

			std::vector<PassNode*> nodesToSyncWith; // ִ�и�PassNodeǰ��Ҫͬ���ȴ�������PassNode
		};

		class DependencyLevel {
		public:

		private:
			std::vector<PassNode*> mPassNodes;
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

	private:
		RingFrameTracker* mFrameTracker{ nullptr };

		bool mBuilded{ false };
		std::vector<std::unique_ptr<RenderGraphPass>> mRenderGraphPasses;
		std::vector<std::unique_ptr<PassNode>> mPassNodes;

		std::unordered_map<std::string, std::unique_ptr<RGTexture>> mTextures;
		std::unordered_map<std::string, std::unique_ptr<RGBuffer>>  mBuffers;

	};

}

#include "RenderGraph.inl"