#pragma once
#include <string>
#include <unordered_map>
#include <variant>

#include "Buffer.h"
#include "Texture.h"

namespace Renderer {

	/*
	* 纹理描述
	*/
	struct RGTextureDesc {
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
	struct RGBufferDesc {
		uint32_t               stride = 1u;
		size_t                 size = 0u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Upload;
		GHL::EBufferMiscFlag   miscFlag = GHL::EBufferMiscFlag::None;
	};

	using RGResourceDesc = std::variant<RGTextureDesc, RGBufferDesc>;

	class RenderGraphResource {
	public:
		using ResourceUsageTimeline = std::pair<uint64_t, uint64_t>;

	public:
		RenderGraphResource(const std::string& name, const RGBufferDesc& desc, bool imported = false)
		: mResName(name)
		, mImported(imported)
		, mResourceDesc(desc) {}

		RenderGraphResource(const std::string& name, const RGTextureDesc& desc, bool imported = false)
		: mResName(name)
		, mImported(imported)
		, mResourceDesc(desc) {}

		RenderGraphResource(const std::string& name, Texture* importedTexture, bool imported = true)
		: mResName(name)
		, mImported(imported) 
		, mTexture(importedTexture) {}

		RenderGraphResource(const std::string& name, Buffer* importedBuffer, bool imported = true)
		: mResName(name)
		, mImported(imported) 
		, mBuffer(importedBuffer) {}

		~RenderGraphResource() = default;

		void ApplyInitialStates(GHL::EResourceState initialState);

		void ApplyExpectedStates(GHL::EResourceState expectedState);

		void StartTimeline(uint64_t nodeExecutionIndex);

		void UpdateTimeline(uint64_t nodeExecutionIndex);

	private:
		std::string mResName;            // 资源名称
		bool mImported{ false };         // 资源是否来自于外部导入
		ResourceUsageTimeline mTimeline; // 资源的生命周期

		Texture* mTexture{ nullptr };
		Buffer*  mBuffer{ nullptr };

		RGResourceDesc mResourceDesc{};

		GHL::EResourceState mInitialStates;
		GHL::EResourceState mExpectedStates;
	};

}