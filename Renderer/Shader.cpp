#include "Shader.h"
#include "ShaderManger.h"

#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace Renderer {

	GraphicsShader::GraphicsShader(ShaderManger* manger)
	: mManger(manger) {}

	GraphicsShader::TextureSlot::TextureSlot(const std::string& name, Texture* texture)
	: name(name)
	, texture(texture) {}

	GraphicsShader::Uniform::Uniform(const std::string& name, Type type, size_t offset, void* data)
	: name(name)
	, type(type)
	, offset(offset) {
		if (data != nullptr) {
			switch (type) {
			case Uniform::Type::Float:  value.v1    = *reinterpret_cast<float*>(data);         break;
			case Uniform::Type::Float2: value.v2    = *reinterpret_cast<Math::Vector2*>(data); break;
			case Uniform::Type::Float3: value.v3    = *reinterpret_cast<Math::Vector3*>(data); break;
			case Uniform::Type::Float4: value.v4    = *reinterpret_cast<Math::Vector4*>(data); break;
			case Uniform::Type::Color:  value.color = *reinterpret_cast<Math::Color*>(data);   break;
			default:
				ASSERT_FORMAT(false, "Unsupported Uniform Type");
				break;
			}
		}
	}

	void GraphicsShader::EmplaceTexture(const std::string& name, Texture* defaultTexture) {
		// 同名检测在上层调用时完成，这里不做同名检测
		mTextureSlots.emplace_back(name, defaultTexture);
	}

	void GraphicsShader::EmplaceUniform(const std::string& name, const Uniform::Type& type, void* data) {
		// 同名检测在上层调用时完成，这里不做同名检测
		size_t offset = 16u * mUniforms.size(); // 一个uniform占用16字节

		mUniforms.emplace_back(name, type, offset, data);
	}

	void GraphicsShader::AttachMaterial(Material* material) {
		auto it = std::find_if(mAssociatedMaterials.begin(), mAssociatedMaterials.end(),
			[&](Material* item) {
				if (item == material) {
					return true;
				}
				return false;
			});

		// material已存在
		if (it != mAssociatedMaterials.end()) {
			return;
		}

		mAssociatedMaterials.push_back(material);
	}

	void GraphicsShader::DetachMaterial(Material* material) {
		auto it = std::find_if(mAssociatedMaterials.begin(), mAssociatedMaterials.end(),
			[&](Material* item) {
				if (item == material) {
					return true;
				}
				return false;
			});

		// material不存在
		if (it == mAssociatedMaterials.end()) {
			return;
		}

		mAssociatedMaterials.erase(it);
	}

	void GraphicsShader::CompileInternalD3DObject() {
		// 编译GHL::Shader
		ASSERT_FORMAT(!mProxy.vsFilepath.empty(), "VSFilepath Must not be Empty");
		ASSERT_FORMAT(!mProxy.psFilepath.empty(), "PSFilepath Must not be Empty");

		GHL::ShaderDesc shaderDesc{};
		shaderDesc.model = GHL::EShaderModel::SM_6_6;
		
		// VS
		shaderDesc.file = mProxy.vsFilepath;
		shaderDesc.entryPoint = mProxy.vsEntryPoint ? *mProxy.vsEntryPoint : "VSMain";
		shaderDesc.stage = GHL::EShaderStage::VS;

		GHL::ShaderCompilationResult compilationResult = mManger->GetCompiler().CompileShader(shaderDesc);
		ASSERT_FORMAT(compilationResult.compiledShader.GetPointer() != nullptr, "Compiled Blob is nullptr");
		mInternalShaders[GHL::EShaderStage::VS] = std::make_unique<GHL::Shader>(compilationResult.compiledShader);

		// PS
		shaderDesc.file = mProxy.psFilepath;
		shaderDesc.entryPoint = mProxy.psEntryPoint ? *mProxy.psEntryPoint : "PSMain";
		shaderDesc.stage = GHL::EShaderStage::PS;

		compilationResult = mManger->GetCompiler().CompileShader(shaderDesc);
		ASSERT_FORMAT(compilationResult.compiledShader.GetPointer() != nullptr, "Compiled Blob is nullptr");
		mInternalShaders[GHL::EShaderStage::PS] = std::make_unique<GHL::Shader>(compilationResult.compiledShader);
		
		// GS
		if (mProxy.gsFilepath) {
			shaderDesc.file = *mProxy.gsFilepath;
			shaderDesc.entryPoint = mProxy.gsEntryPoint ? *mProxy.gsEntryPoint : "GSMain";
			shaderDesc.stage = GHL::EShaderStage::GS;

			compilationResult = mManger->GetCompiler().CompileShader(shaderDesc);
			ASSERT_FORMAT(compilationResult.compiledShader.GetPointer() != nullptr, "Compiled Blob is nullptr");
			mInternalShaders[GHL::EShaderStage::GS] = std::make_unique<GHL::Shader>(compilationResult.compiledShader);
		}

		// 编译GHL::PipelineState
		mInternalPipelineState = std::make_unique<GHL::GraphicsPipelineState>(mManger->GetDevice());

		mInternalPipelineState->desc.SampleMask = mProxy.sampleMask;
		mInternalPipelineState->desc.SampleDesc = mProxy.sampleDesc;
		mInternalPipelineState->desc.BlendState = mProxy.blendDesc;
		mInternalPipelineState->desc.RasterizerState = mProxy.rasterizerDesc;
		mInternalPipelineState->desc.DepthStencilState = mProxy.depthStencilDesc;
		mInternalPipelineState->desc.DSVFormat = mProxy.depthStencilFormat;
		mInternalPipelineState->desc.PrimitiveTopologyType = mProxy.primitiveTopology;
		mInternalPipelineState->desc.NumRenderTargets = mProxy.renderTargetFormatArray.size();
		for (uint32_t i = 0; i < mInternalPipelineState->desc.NumRenderTargets; i++) {
			mInternalPipelineState->desc.RTVFormats[i] = mProxy.renderTargetFormatArray[i];
		}
		mInternalPipelineState->desc.InputLayout.pInputElementDescs = mProxy.inputElementArray.data();
		mInternalPipelineState->desc.InputLayout.NumElements = mProxy.inputElementArray.size();
		mInternalPipelineState->desc.pRootSignature = mManger->GetBaseD3DRootSignature();

		mInternalPipelineState->desc.VS = mInternalShaders.at(GHL::EShaderStage::VS)->D3DBytecode();
		mInternalPipelineState->desc.PS = mInternalShaders.at(GHL::EShaderStage::PS)->D3DBytecode();

		if (mProxy.gsFilepath) {
			mInternalPipelineState->desc.GS = mInternalShaders.at(GHL::EShaderStage::GS)->D3DBytecode();
		}

		mInternalPipelineState->Compile();
		mInternalPipelineState->SetDebugName(mProxy.name);
	}

	void GraphicsShader::SetPipelineStateProxy(const GraphicsStateProxy& proxy) {
		mProxy = proxy;
	}

	ComputeShader::ComputeShader(ShaderManger* manger)
	: mManger(manger) {}

	void ComputeShader::CompileInternalD3DObject() {
		// 编译GHL::Shader
		ASSERT_FORMAT(!mProxy.csFilepath.empty(), "CSFilepath Must not be Empty");

		GHL::ShaderDesc shaderDesc{};
		shaderDesc.model = GHL::EShaderModel::SM_6_6;

		// CS
		shaderDesc.file = mProxy.csFilepath;
		shaderDesc.entryPoint = mProxy.csEntryPoint ? *mProxy.csEntryPoint : "CSMain";
		shaderDesc.stage = GHL::EShaderStage::CS;

		GHL::ShaderCompilationResult compilationResult = mManger->GetCompiler().CompileShader(shaderDesc);
		ASSERT_FORMAT(compilationResult.compiledShader.GetPointer() != nullptr, "Compiled Blob is nullptr");
		mInternalShaders[GHL::EShaderStage::CS] = std::make_unique<GHL::Shader>(compilationResult.compiledShader);

		// 编译GHL::PipelineState
		mInternalPipelineState = std::make_unique<GHL::ComputePipelineState>(mManger->GetDevice());

		mInternalPipelineState->desc.CS = mInternalShaders.at(GHL::EShaderStage::CS)->D3DBytecode();
		mInternalPipelineState->desc.pRootSignature = mManger->GetBaseD3DRootSignature();

		mInternalPipelineState->Compile();
		mInternalPipelineState->SetDebugName(mProxy.name);
	}

	void ComputeShader::SetPipelineStateProxy(const ComputeStateProxy& proxy) {
		mProxy = proxy;
	}

}