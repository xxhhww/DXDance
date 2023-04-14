#pragma once
#include "GHL/pbh.h"
#include <variant>

namespace Renderer {

	/*
	* ��������
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
	* ��������
	*/
	struct NewBufferProperties {
		uint32_t               stride = 0u;
		size_t                 size = 0u;
		DXGI_FORMAT            format = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage = GHL::EResourceUsage::Default;
		GHL::EBufferMiscFlag   miscFlag = GHL::EBufferMiscFlag::None;
	};

	/*
	* ��ɫ������Դ�ķ��ʱ�ʶ��
	*/
	enum class ShaderAccessFlag {
		NonPixelShader, // ��������ɫ������
		PixelShader,    // ������ɫ������
		AnyShader       // ������ɫ�����ɷ���
	};

	using NewResourceProperties = std::variant<NewTextureProperties, NewBufferProperties>;
}