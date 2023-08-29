#pragma once
#include "Math/Int.h"

namespace Renderer {

	/*
	* Cell数据
	*/
	struct RVTPagePayload {
	public:
        inline static Math::Int2 smInvalidTileIndex = Math::Int2(-1, -1);

        // 对应平铺贴图中的id
        Math::Int2 tileIndex{ smInvalidTileIndex };

        // 激活的帧序号
        // public int ActiveFrame;

        // 渲染请求
        // public RenderTextureRequest LoadRequest;

    public:
        // 是否处于可用状态
        inline bool IsReady() {
            return (tileIndex != smInvalidTileIndex);
        }

        // 重置数据
        inline void Reset() {
            tileIndex = smInvalidTileIndex;
        }
	};

}