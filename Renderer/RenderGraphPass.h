#pragma once
#include "RenderGraphBuilder.h"

#include <functional>
#include <string>

namespace Renderer {

	class RenderGraphPass {
	public:
		using SetupFunc   = std::function<void(RenderGraphBuilder& builer)>;
		using ExecuteFunc = std::function<void>;

	public:
		RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute);
		~RenderGraphPass() = default;

	private:
		std::string mName;
		SetupFunc   mSetup;
		ExecuteFunc mExecute;
	};

}