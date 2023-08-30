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

        // ��Ӧƽ����ͼ�е�id
        Math::Int2 tileIndex{ smInvalidTileIndex };

        // �����֡���
        uint64_t activeFrame;

        // ״̬
        CellState cellState{ CellState::InActive };

        // ��������
        std::optional<DrawTileRequest> drawRequest{ std::nullopt };

    public:
        // �Ƿ��ڿ���״̬
        inline bool IsReady() {
            return (tileIndex != smInvalidTileIndex);
        }

        // ��������
        inline void Reset() {
            tileIndex = smInvalidTileIndex;
        }
	};

}