#pragma once
#include "ResourceFormat.h"
#include <string>
#include <unordered_map>

namespace Renderer {

	class Buffer;
	class Texture;

	/*
	* 纹理描述
	*/
	struct NewTextureProperties {
		GHL::ETextureDimension dimension = GHL::ETextureDimension::Texture2D;
		uint32_t               width = 0u;
		uint32_t               height = 0u;
		uint32_t               depth = 1u;
		uint32_t               arraySize = 1u;
		uint32_t               mipLevals = 1u;
		uint32_t               sampleCount = 1u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Default;
		GHL::ETextureMiscFlag  miscFlag = GHL::ETextureMiscFlag::None;
		D3D12_CLEAR_VALUE      clearValue{};
	};

	/*
	* 缓冲描述
	*/
	struct NewBufferProperties {
		uint32_t               stride = 0u;
		size_t                 size = 0u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Upload;
		GHL::EBufferMiscFlag   miscFlag = GHL::EBufferMiscFlag::None;
	};

	/*
	* 着色器对资源的访问标识符
	*/
	enum class ShaderAccessFlag {
		NonPixelShader, // 非像素着色器访问
		PixelShader,    // 像素着色器访问
		AnyShader       // 所有着色器均可访问
	};

	using NewResourceProperties = std::variant<NewTextureProperties, NewBufferProperties>;

	class RenderGraphResource {
	public:
		using ResourceUsageTimeline = std::pair<uint64_t, uint64_t>;

	public:
		RenderGraphResource(const GHL::Device* device, const std::string& name);
		RenderGraphResource(const std::string& name, Texture* resource);
		RenderGraphResource(const std::string& name, Buffer*  resource);
		~RenderGraphResource() = default;

		/*
		* 构建ResourceFormat，获取D3D12ResourceDesc 与 显存要求
		*/
		void BuildResourceFormat();

		/*
		* 在堆上创建资源
		*/
		void BuildPlacedResource();

		void StartTimeline(uint64_t nodeGlobalExecutionIndex);

		void UpdateTimeline(uint64_t nodeGlobalExecutionIndex);

		void SetNewTextureProperties(const NewTextureProperties& properties);

		void SetNewBufferProperties(const NewBufferProperties& properties);

		void SetInitialStates(GHL::EResourceState states);

		void SetExpectedStates(uint64_t nodeIndex, GHL::EResourceState states);

		inline const auto& GetUsageTimeline()  const { return mTimeline; }
		inline const auto& GetRequiredMemory() const { return mResourceFormat.GetSizeInBytes(); }

	public:
		size_t heapOffset{ 0u };
		bool aliased{ false };

	private:
		const GHL::Device* mDevice{ nullptr };
		std::string mResName; // 资源名称

		Texture* mTexture{ nullptr };
		Buffer*  mBuffer { nullptr };

		// 以下是资源调度信息

		bool mImported{ false };         // 资源是否来自于外部导入
		ResourceUsageTimeline mTimeline; // 资源的生命周期
		NewResourceProperties mNewResourceProperties;
		GHL::EResourceState mInitialStates{ GHL::EResourceState::Common };
		GHL::EResourceState mExpectedStates{ GHL::EResourceState::Common };
		std::unordered_map<uint64_t, GHL::EResourceState> mExpectedStatesPerPass; // 各个Pass对该资源的要求

		ResourceFormat mResourceFormat;
	};

}