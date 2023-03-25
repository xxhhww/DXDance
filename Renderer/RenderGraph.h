#pragma once

#include "RingFrameTracker.h"

#include <memory>
#include <unordered_map>
#include <string>

namespace Renderer {

	class RenderPass;

	class RenderGraph {
	public:
		class PassNode {
		public:

		private:
			RenderPass* mRenderPass{ nullptr };

		};

		class DependencyLevel {
		public:

		private:
		};

	public:
		RenderGraph(RingFrameTracker* frameTracker);
		~RenderGraph() = default;

		/*
		* ���RenderPass
		*/
		void AddRenderPass(const std::string& name, std::unique_ptr<RenderPass>& renderPass);

		/*
		* ������Ⱦ����ͼ
		*/
		void BuildGraph();

	private:
		RingFrameTracker* mFrameTracker{ nullptr };

		std::unordered_map<std::string, std::unique_ptr<RenderPass>> mRenderPasses;

	};

}