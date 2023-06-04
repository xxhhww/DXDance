#pragma once
#include <functional>
#include <string>

namespace Renderer {

	class RenderGraphBuilder;
	class ShaderManger;
	class CommandSignatureManger;
	class CommandBuffer;
	struct RenderContext;

	class RenderGraphPass {
	public:
		using SetupFunc   = std::function<void(RenderGraphBuilder&, ShaderManger&, CommandSignatureManger&)>;
		using ExecuteFunc = std::function<void(CommandBuffer&, RenderContext&)>;

	public:
		RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute);
		~RenderGraphPass() = default;

		void SetUp(RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger);
		void Execute(CommandBuffer& commandBuffer, RenderContext& context);

		inline const std::string& GetName() const { return mName; }

	private:
		std::string mName;
		SetupFunc   mSetup;
		ExecuteFunc mExecute;
	};

}