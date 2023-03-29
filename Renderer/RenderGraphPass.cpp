#include "RenderGraphPass.h"
#include "RenderGraphBuilder.h"

namespace Renderer {

	RenderGraphPass::RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute)
	: mName(name)
	, mSetup(std::forward<SetupFunc>(setup))
	, mExecute(std::forward<ExecuteFunc>(execute)) {}


	void RenderGraphPass::SetUp(RenderGraphBuilder& builder) {
		mSetup(builder);
	}

	void RenderGraphPass::Execute() {
		mExecute();
	}

}