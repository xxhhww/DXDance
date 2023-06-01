#include "RenderGraphPass.h"
#include "RenderGraphBuilder.h"
#include "CommandBuffer.h"

namespace Renderer {

	RenderGraphPass::RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute)
	: mName(name)
	, mSetup(std::forward<SetupFunc>(setup))
	, mExecute(std::forward<ExecuteFunc>(execute)) {}


	void RenderGraphPass::SetUp(RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
		mSetup(builder, shaderManger, commandSignatureManger);
	}

	void RenderGraphPass::Execute(CommandBuffer& commandBuffer, RenderContext& context) {
		mExecute(commandBuffer, context);
	}

}