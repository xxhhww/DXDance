#pragma once
#include "Math/Int.h"
#include <optional>

namespace Renderer {

    enum class CellState {
        InActive = 0x00,
        Loading = 0x01,
        Active = 0x02
    };

    struct DrawTileRequest {
    public:
        int x{ 0 };
        int y{ 0 };
        int mipLevel{ 0 };

    public:
        inline DrawTileRequest() = default;
        inline DrawTileRequest(int x, int y, int mipLevel) : x(x), y(y), mipLevel(mipLevel) {}
    };

	struct RvtPagePayload {
	public:
        inline static Math::Int2 smInvalidTileIndex = Math::Int2(-1, -1);

        // 对应平铺贴图中的id
        Math::Int2 tileIndex{ smInvalidTileIndex };

        // 激活的帧序号
        uint64_t activeFrame;

        // 状态
        CellState cellState{ CellState::InActive };

        // 绘制请求
        std::optional<DrawTileRequest> drawRequest{ std::nullopt };

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