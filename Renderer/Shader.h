#pragma once
#include "GHL/Shader.h"
#include "GHL/PipelineState.h"
#include "Texture.h"

#include "Math/Color.h"

#include <unordered_map>
#include <unordered_set>
#include <variant>
#include <memory>

namespace Renderer {

	class Material;

	class ShaderManger;

	/*
	* 着色器接口
	* 一个着色器就是一个完成的管线流程
	*/
	class IShader {
	public:
		virtual ~IShader() = default;
	};

	class GraphicsStateProxy {
	public:

		GraphicsStateProxy() {
			// 部分初始化
			sampleMask = UINT_MAX;
			blendDesc = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
			sampleDesc.Count = 1u;
			sampleDesc.Quality = 0u;
			rasterizerDesc = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
			depthStencilDesc = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
			depthStencilFormat = DXGI_FORMAT_UNKNOWN;
			primitiveTopology = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
			renderTargetFormatArray = { DXGI_FORMAT_R8G8B8A8_UNORM };
			inputElementArray = {
				{ "POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
				{ "NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
				{ "TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
			};

			rootSignatureName = "Signature_Base";
		}

		~GraphicsStateProxy() = default;

		std::string name;

		std::string vsFilepath;
		std::string psFilepath;
		std::optional<std::string> gsFilepath;
		std::optional<std::string> vsEntryPoint;
		std::optional<std::string> psEntryPoint;
		std::optional<std::string> gsEntryPoint;

		UINT sampleMask;
		D3D12_BLEND_DESC blendDesc;
		DXGI_SAMPLE_DESC sampleDesc;
		D3D12_RASTERIZER_DESC rasterizerDesc;
		D3D12_DEPTH_STENCIL_DESC depthStencilDesc;
		DXGI_FORMAT depthStencilFormat;
		D3D12_PRIMITIVE_TOPOLOGY_TYPE primitiveTopology;
		std::vector<DXGI_FORMAT> renderTargetFormatArray;
		std::vector<D3D12_INPUT_ELEMENT_DESC> inputElementArray;

		std::string rootSignatureName;
	};

	/*
	* 图形着色器
	*/
	class GraphicsShader : public IShader {
	public:
		/*
		* 纹理插槽
		*/
		struct TextureSlot {
		public:
			TextureSlot(const std::string& name, Texture* texture = nullptr);
			~TextureSlot() = default;

			std::string name{ "" };
			Texture*    texture{ nullptr };
		};

		/*
		* 常量缓冲(ConstantBuffer)中的标准元素，注意一个ConstantBuffer以256字节为偏移量
		* 这里规定一个Uniform占用16字节
		* float : (4, null, null, null)
		* float2: (4, 4, null, null)
		* float3: (4, 4, 4, null)
		* float4: (4, 4, 4, 4)
		*/
		struct Uniform {
		public:
			enum class Type {
				Float, Float2, Float3, Float4, Color
			};

			union Var {
				float         v1;
				Math::Vector2 v2;
				Math::Vector3 v3;
				Math::Vector4 v4;
				Math::Color   color;
			};
		public:
			Uniform(const std::string& name, Type type, size_t offset, void* data = nullptr);
			~Uniform() = default;

		public:
			std::string name{ "" };
			Type        type{ Type::Float };
			size_t      offset{ 0u };
			Var         value{};
		};

	public:
		GraphicsShader(ShaderManger* manger);
		~GraphicsShader() = default;

		void EmplaceTexture(const std::string& name, Texture* defaultTexture = nullptr);

		void EmplaceUniform(const std::string& name, const Uniform::Type& type, void* data = nullptr);

		void AttachMaterial(Material* material);

		void DetachMaterial(Material* material);

		/*
		* 编译内部D3D对象(D3DShader 与 D3DPSO)
		*/
		void CompileInternalD3DObject();

		void SetPipelineStateProxy(const GraphicsStateProxy& proxy);

		inline const auto* GetPipelineState()        const { return mInternalPipelineState.get(); }
		inline const auto* GetD3DPipelineState()     const { return mInternalPipelineState->D3DPipelineState(); }
		inline const auto& GetD3DPipelineStateDesc() const { return mInternalPipelineState->desc; }
		
	private:
		ShaderManger* mManger{ nullptr };

		bool mHasCompiled{ false }; // 着色器是否编译过一次

		std::unordered_map<GHL::EShaderStage, std::unique_ptr<GHL::Shader>> mInternalShaders; // 与该着色器关联的D3D着色器
		std::unordered_set<std::string> mIncludedFilePaths; // HLSL文件包含的头文件

		GraphicsStateProxy mProxy{};
		std::unique_ptr<GHL::GraphicsPipelineState> mInternalPipelineState; // 与该着色器关联的D3DPSO

		// ========================...以下为自定义编辑的相关变量...========================

		std::vector<TextureSlot> mTextureSlots;
		std::vector<Uniform> mUniforms;

		std::vector<Material*> mAssociatedMaterials; // 与该着色器相关联的材质
	};

	class ComputeStateProxy {
	public:

		ComputeStateProxy() {

			rootSignatureName = "Signature_Base";
		}

		~ComputeStateProxy() = default;

		std::string name;

		std::string csFilepath;
		std::optional<std::string> csEntryPoint;

		std::string rootSignatureName;
	};

	/*
	* 计算着色器
	*/
	class ComputeShader : public IShader {
	public:
		ComputeShader(ShaderManger* manger);
		~ComputeShader() = default;

		/*
		* 编译内部D3D对象(D3DShader 与 D3DPSO)
		*/
		void CompileInternalD3DObject();

		void SetPipelineStateProxy(const ComputeStateProxy& proxy);

		inline const auto* GetPipelineState()        const { return mInternalPipelineState.get(); }
		inline const auto* GetD3DPipelineState()     const { return mInternalPipelineState->D3DPipelineState(); }
		inline const auto& GetD3DPipelineStateDesc() const { return mInternalPipelineState->desc; }

	private:
		ShaderManger* mManger{ nullptr };

		bool mHasCompiled{ false };

		std::unordered_map<GHL::EShaderStage, std::unique_ptr<GHL::Shader>> mInternalShaders;
		std::unordered_set<std::string> mIncludedFilePaths;

		ComputeStateProxy mProxy{};
		std::unique_ptr<GHL::ComputePipelineState> mInternalPipelineState;
	};
}