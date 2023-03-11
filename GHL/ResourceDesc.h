#pragma once
#include "pbh.h"

namespace GHL {

	struct BufferDesc {
		uint32_t          stride   = 1u;	                  // 缓冲中的元素的步幅
		size_t            size     = 0u;	                  // 缓冲的总字节个数
		EResourceBindFlag bindFlag = EResourceBindFlag::None; // 缓冲需要绑定的视图类型
		EBufferMiscFlag   miscFlag = EBufferMiscFlag::None;   // 缓冲的杂项标记
	};

}