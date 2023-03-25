#pragma once
#include "Shader.h"
#include "GHL/ShaderCompiler.h"
#include "GHL/RootSignature.h"

namespace Renderer {

	class ShaderManger {
	public:
		using GraphicsStateConfigurator = std::function<void(GraphicsStateProxy&)>;
		using ComputeStateConfigurator = std::function<void(ComputeStateProxy&)>;

	public:
		ShaderManger(const GHL::Device* device);
		~ShaderManger() = default;

		void CreateGraphicsShader(const std::string& name, const GraphicsStateConfigurator& configurator);

		void CreateComputeShader(const std::string& name, const ComputeStateConfigurator& configurator);

		inline const auto* GetDevice()               const { return mDevice; }
		inline const auto& GetCompiler()             const { return mCompiler; }

		inline auto* GetBaseD3DRootSignature() const { return mBaseRootSignature->D3DRootSignature(); }

	private:
		const GHL::Device* mDevice{ nullptr };

		GHL::ShaderCompiler mCompiler;

		std::unique_ptr<GHL::RootSignature> mBaseRootSignature;

		std::unordered_map<std::string, std::unique_ptr<Renderer::IShader>> mShaders;
	};

}