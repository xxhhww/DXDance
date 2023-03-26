#include "RenderGraphPass.h"

namespace Renderer {

	RenderGraphPass::RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute)
	: mName(name)
	, mSetup(std::forward<SetupFunc>(setup))
	, mExecute(std::forward<ExecuteFunc>(execute)) {}

}