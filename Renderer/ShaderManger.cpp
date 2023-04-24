#include "ShaderManger.h"

namespace Renderer {

	ShaderManger::ShaderManger(const GHL::Device* device)
	: mDevice(device)
	, mCompiler() {
		mBaseRootSignature = std::make_unique<GHL::RootSignature>(mDevice);
		mBaseRootSignature->InitStaticSampler();
		mBaseRootSignature->AddRootDescriptorParameter(GHL::RootConstantBufferParameter{ 0u, 10u });
		mBaseRootSignature->Compile();
	}

	void ShaderManger::CreateGraphicsShader(const std::string& name, const GraphicsStateConfigurator& configurator) {
		auto shader = std::make_unique<Renderer::GraphicsShader>(this);

		GraphicsStateProxy proxy;
		configurator(proxy);

		shader->SetPipelineStateProxy(proxy);
		shader->CompileInternalD3DObject();

		mShaders[name] = std::move(shader);
	}

	void ShaderManger::CreateComputeShader(const std::string& name, const ComputeStateConfigurator& configurator) {
		auto shader = std::make_unique<Renderer::ComputeShader>(this);

		ComputeStateProxy proxy;
		configurator(proxy);

		shader->SetPipelineStateProxy(proxy);
		shader->CompileInternalD3DObject();

		mShaders[name] = std::move(shader);
	}

}