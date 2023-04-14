#pragma once
#include "GHL/pbh.h"
#include <variant>

namespace Renderer {

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
		GHL::ClearValue        clearValue = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
	};

	/*
	* 缓冲描述
	*/
	struct NewBufferProperties {
		uint32_t               stride = 0u;
		size_t                 size = 0u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Default;
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
}