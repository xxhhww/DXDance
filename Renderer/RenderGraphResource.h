#pragma once
#include "ResourceFormat.h"
#include <string>
#include <unordered_map>

namespace GHL {

	class Heap;

}

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

		/*
		* Pass对该资源的请求信息
		*/
		struct PassRequestedInfo {
		public:
			GHL::EResourceState expectedStates{ GHL::EResourceState::Common };
			bool needAliased{ false };
		};

	public:
		RenderGraphResource(const GHL::Device* device, const std::string& name);
		RenderGraphResource(const std::string& name, Texture* resource);
		RenderGraphResource(const std::string& name, Buffer*  resource);
		~RenderGraphResource() = default;

		/*
		* 构建ResourceFormat，获取D3D12ResourceDesc 与 显存要求
		*/
		void BuildResourceFormat();

		void StartTimeline(uint64_t nodeGlobalExecutionIndex);

		void UpdateTimeline(uint64_t nodeGlobalExecutionIndex);

		void SetInitialStates(GHL::EResourceState states);

		void SetExpectedStates(uint64_t nodeIndex, GHL::EResourceState states);

		/*
		* 为使用该资源的第一个Pass(也就是创建该资源的Pass)设置needAliased标识符
		*/
		void SetAliasedForFirstPass();

		inline const auto& GetUsageTimeline()  const { return mTimeline; }
		inline const auto& GetRequiredMemory() const { return resourceFormat.GetSizeInBytes(); }

	public:
		bool aliased{ false };
		bool imported{ false };

		GHL::Heap* heap{ nullptr };
		size_t heapOffset{ 0u };

		Texture* texture{ nullptr };
		Buffer*  buffer { nullptr };
		NewResourceProperties newResourceProperties;
		ResourceFormat resourceFormat;

	private:
		const GHL::Device* mDevice{ nullptr };
		std::string mResName; // 资源名称

		// 以下是资源调度信息

		ResourceUsageTimeline mTimeline; // 资源的生命周期
		GHL::EResourceState mInitialStates{ GHL::EResourceState::Common };
		GHL::EResourceState mExpectedStates{ GHL::EResourceState::Common };
		std::unordered_map<uint64_t, PassRequestedInfo> mRequestedInfoPerPass; // 各个Pass对该资源的要求
	};

}