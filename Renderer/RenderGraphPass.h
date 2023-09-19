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
		using SetupFunc = std::function<void(RenderGraphBuilder&, ShaderManger&, CommandSignatureManger&)>;
		using ExecuteFunc = std::function<void(CommandBuffer&, RenderContext&)>;
		using BeginFunc = std::function<void()>;
		using EndFunc = std::function<void()>;

	public:
		RenderGraphPass(const std::string& name, SetupFunc&& setup, ExecuteFunc&& execute, BeginFunc&& begin = nullptr, EndFunc&& end = nullptr);
		~RenderGraphPass() = default;

		void SetUp(RenderGraphBuilder& builder, ShaderManger& shaderManger, CommandSignatureManger& commandSignatureManger);
		void Execute(CommandBuffer& commandBuffer, RenderContext& context);
		void Begin();
		void End();

		inline const std::string& GetName() const { return mName; }

	private:
		std::string	mName;
		SetupFunc	mSetup;
		BeginFunc   mBegin;
		ExecuteFunc	mExecute;
		EndFunc     mEnd;
	};

}