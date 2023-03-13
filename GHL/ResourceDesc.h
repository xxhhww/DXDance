#pragma once
#include "pbh.h"

namespace GHL {

	struct BufferDesc {
		uint32_t          stride        = 1u;	                   // 缓冲中的元素的步幅
		size_t            size          = 0u;	                   // 缓冲的总字节个数
		DXGI_FORMAT       format        = DXGI_FORMAT_UNKNOWN;     // 缓冲元素的格式
		EResourceUsage    usage         = EResourceUsage::Upload;  // 资源类型
		EResourceBindFlag bindFlag      = EResourceBindFlag::None; // 缓冲需要绑定的视图类型(待弃用)
		EBufferMiscFlag   miscFlag      = EBufferMiscFlag::None;   // 缓冲的杂项标记
		EResourceState    initalState   = EResourceState::Common;  // 所有初始的状态
		EResourceState    expectedState = EResourceState::Common;  // 所有期望的状态
		bool              placed        = false;                   // 是否使用placed的方式创建缓冲
	};

	/*
	* 一般用不到
	*/
	struct BufferSubResourceDesc {
		uint64_t offset = 0u;
		uint64_t size   = uint64_t(-1);
	};

	struct TextureDesc {
		ETextureDimension dimension = ETextureDimension::Texture2D; // 纹理的维度
		uint32_t          width         = 0u;
		uint32_t          height        = 0u;
		uint32_t          depth         = 0u;
		uint32_t          arraySize     = 1u;
		uint32_t          mipLevals     = 1u;
		uint32_t          sampleCount   = 1u;
		DXGI_FORMAT       format        = DXGI_FORMAT_UNKNOWN;
		EResourceUsage    usage         = EResourceUsage::Default;
		ETextureMiscFlag  miscFlag      = ETextureMiscFlag::None;
		EResourceBindFlag bindFlag      = EResourceBindFlag::None;  // (待弃用)
		EResourceState    initialState  = EResourceState::Common;
		EResourceState    expectedState = EResourceState::Common;
		bool              placed        = false;                    // 是否使用placed的方式创建纹理
		bool              reserved      = false;                    // 是否使用reserved的方式创建纹理
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