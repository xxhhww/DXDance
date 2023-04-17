#pragma once
#include "PoolCommandListAllocator.h"

#include <functional>
#include <string>

namespace Renderer {

	class RenderGraphBuilder;
	class ShaderManger;
	struct RenderContext;

	class RenderGraphPass {
	public:
		using SetupFunc   = std::function<void(RenderGraphBuilder& builer, ShaderManger& manger)>;
		using ExecuteFunc = std::function<void(CommandListWrap& commandList, RenderContext& context)>;

	public:
		RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute);
		~RenderGraphPass() = default;

		void SetUp(RenderGraphBuilder& builder, ShaderManger& manger);
		void Execute(CommandListWrap& commandList, RenderContext& context);

	private:
		std::string mName;
		SetupFunc   mSetup;
		ExecuteFunc mExecute;
	};

}