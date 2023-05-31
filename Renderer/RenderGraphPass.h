#pragma once
#include "PoolCommandListAllocator.h"

#include <functional>
#include <string>

namespace Renderer {

	class RenderGraphBuilder;
	class ShaderManger;
	class CommandSignatureManger;
	struct RenderContext;

	class RenderGraphPass {
	public:
		using SetupFunc   = std::function<void(RenderGraphBuilder&, ShaderManger&, CommandSignatureManger&)>;
		using ExecuteFunc = std::function<void(CommandListWrap&, RenderContext&)>;

	public:
		RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute);
		~RenderGraphPass() = default;

		void SetUp(RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger);
		void Execute(CommandListWrap& commandList, RenderContext& context);

	private:
		std::string mName;
		SetupFunc   mSetup;
		ExecuteFunc mExecute;
	};

}