#pragma once
#include "GHL/pbh.h"

namespace Renderer {

	/*
	* ��������
	*/
	struct TextureDesc {
		GHL::ETextureDimension dimension     = GHL::ETextureDimension::Texture2D; // �����ά��
		uint32_t               width         = 0u;
		uint32_t               height        = 0u;
		uint32_t               depth         = 1u;                                // ������ά������άʱ������depth���
		uint32_t               arraySize     = 1u;
		uint32_t               mipLevals     = 1u;
		uint32_t               sampleCount   = 1u;                                // (������)
		DXGI_FORMAT            format        = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage         = GHL::EResourceUsage::Default;      // ������Ҫ��Default���Ͻ��д���
		GHL::ETextureMiscFlag  miscFlag      = GHL::ETextureMiscFlag::None;
		GHL::EResourceState    initialState  = GHL::EResourceState::Common;
		GHL::EResourceState    expectedState = GHL::EResourceState::Common;
		GHL::ClearValue        clearValue    = GHL::ColorClearValue{};
		bool                   reserved      = false;                             // �Ƿ�ʹ�ñ�����ʽ����������Դ
	};

	/*
	* ��������Դ����
	*/
	struct TextureSubResourceDesc {
	public:

		bool operator==(const TextureSubResourceDesc& other) const {
			return firstSlice == other.firstSlice && sliceCount == other.sliceCount && firstMip == other.firstMip && other.mipCount == other.mipCount;
		}

		uint32_t firstSlice = 0u;
		uint32_t sliceCount = static_cast<uint32_t>(-1); // -1 ��ʾȫ��
		uint32_t firstMip   = 0u;
		uint32_t mipCount   = static_cast<uint32_t>(-1); // -1 ��ʾȫ��
	};

	struct TextureSubResourceDescHashFunc {
	public:
		std::size_t operator()(const TextureSubResourceDesc& desc) const {
			return std::hash<uint32_t>()(desc.firstSlice) ^ std::hash<uint32_t>()(desc.sliceCount) ^ std::hash<uint32_t>()(desc.firstMip) ^ std::hash<uint32_t>()(desc.mipCount);
		}
	};
}