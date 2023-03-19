#pragma once
#include "GHL/pbh.h"

namespace Renderer {

	/*
	* 纹理描述
	*/
	struct TextureDesc {
		GHL::ETextureDimension dimension     = GHL::ETextureDimension::Texture2D; // 纹理的维度
		uint32_t               width         = 0u;
		uint32_t               height        = 0u;
		uint32_t               depth         = 1u;                                // 当纹理维度是三维时，启用depth标记
		uint32_t               arraySize     = 1u;
		uint32_t               mipLevals     = 1u;
		uint32_t               sampleCount   = 1u;                                // (待弃用)
		DXGI_FORMAT            format        = DXGI_FORMAT_UNKNOWN;
		GHL::EResourceUsage    usage         = GHL::EResourceUsage::Default;      // 纹理需要在Default堆上进行创建
		GHL::ETextureMiscFlag  miscFlag      = GHL::ETextureMiscFlag::None;
		GHL::EResourceState    initialState  = GHL::EResourceState::Common;
		GHL::EResourceState    expectedState = GHL::EResourceState::Common;
		bool                   reserved      = false;                             // 是否使用保留方式创建纹理资源
	};

	/*
	* 纹理子资源描述
	*/
	struct TextureSubResourceDesc {
		uint32_t firstSlice = 0u;
		uint32_t sliceCount = -1; // -1 表示全部
		uint32_t firstMip   = 0u;
		uint32_t mipCount   = -1; // -1 表示全部
	};
}