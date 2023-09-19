#include "RenderGraphPass.h"
#include "RenderGraphBuilder.h"
#include "CommandBuffer.h"

namespace Renderer {

	RenderGraphPass::RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute, BeginFunc&& begin, EndFunc&& end)
	: mName(name)
	, mSetup(std::forward<SetupFunc>(setup))
	, mExecute(std::forward<ExecuteFunc>(execute)) 
	, mBegin(begin)
	, mEnd(end) {}


	void RenderGraphPass::SetUp(RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger) {
		mSetup(builder, shaderManger, commandSignatureManger);
	}

	void RenderGraphPass::Execute(CommandBuffer& commandBuffer, RenderContext& context) {
		mExecute(commandBuffer, context);
	}

	void RenderGraphPass::Begin() {
		if (mBegin != nullptr) {
			mBegin();
		}
	}

	void RenderGraphPass::End() {
		if (mEnd != nullptr) {
			mEnd();
		}
	}

}