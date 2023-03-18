#pragma once
#include "GHL/pbh.h"

namespace Renderer {

	/*
	* 缓冲描述
	*/
	struct BufferDesc {
		uint32_t               stride        = 1u;	                         // 缓冲中的元素的步幅
		size_t                 size          = 0u;	                         // 缓冲的总字节个数
		DXGI_FORMAT            format        = DXGI_FORMAT_UNKNOWN;          // 缓冲元素的格式
		GHL::EResourceUsage    usage         = GHL::EResourceUsage::Upload;  // 资源类型
		GHL::EBufferMiscFlag   miscFlag      = GHL::EBufferMiscFlag::None;   // 缓冲的杂项标记
		GHL::EResourceState    initalState   = GHL::EResourceState::Common;  // 所有初始的状态
		GHL::EResourceState    expectedState = GHL::EResourceState::Common;  // 所有期望的状态
	};

	/*
	* 缓冲子资源描述
	*/
	struct BufferSubResourceDesc {
		size_t offset = 0u;
		size_t size   = static_cast<size_t>(-1);
	};

}