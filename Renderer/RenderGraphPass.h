#pragma once

#include <functional>
#include <string>

namespace Renderer {

	class RenderGraphBuilder;

	class RenderGraphPass {
	public:
		using SetupFunc   = std::function<void(RenderGraphBuilder& builer)>;
		using ExecuteFunc = std::function<void()>;

	public:
		RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute);
		~RenderGraphPass() = default;

		void SetUp(RenderGraphBuilder& builder);
		void Execute();

	private:
		std::string mName;
		SetupFunc   mSetup;
		ExecuteFunc mExecute;
	};

}