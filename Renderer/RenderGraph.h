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
	* Pass的目标GPU队列
	*/
	enum class PassExecutionQueue : uint8_t {
		General = 0, // 通用的图形引擎
		Compute = 1, // 异步计算引擎
		Copy    = 2  // 异步复制引擎
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
			* 添加读写依赖
			*/

			/*
			* 设定PassNode所属的GPU引擎
			*/
			void SetExecutionQueue(PassExecutionQueue queueIndex);

		public:
			RenderGraphPass* pass{ nullptr };    // 该PassNode的内部执行方法
			PassExecutionQueue executionQueueIndex;

			std::unordered_map<std::string, GHL::EResourceState> expectedStateMap; // 该PassNode所使用的资源，及其期望状态

			// 该PassNode的读写依赖，用于构造有向图

			std::unordered_set<std::string> readDependency;
			std::unordered_set<std::string> writeDependency;

			// 以下参数在构造有向图时写入

			uint64_t globalExecutionIndex{ 0u }; // 该PassNode的全局执行顺序
			uint64_t dependencyLevelIndex{ 0u }; // 该PassNode所在的依赖级别
			uint64_t localExecutionIndex{ 0u };  // 该PassNode的局部执行顺序

			std::vector<PassNode*> nodesToSyncWith; // 执行该PassNode前需要同步等待的其他PassNode
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