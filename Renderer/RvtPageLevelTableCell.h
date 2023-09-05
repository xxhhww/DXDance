#pragma once
#include "Math/Int.h"

namespace Renderer {

	enum class CellState {
		InActive = 0x00,
		Loading  = 0x01,
		Active   = 0x02
	};

	struct RvtPageLevelTableCell {
	public:
		inline static Math::Int2 smInvalidTilePos = Math::Int2(-1, -1);

		Math::Int4 rect;
		int        mipLevel;

		Math::Int2 tilePos{ smInvalidTilePos }; // ��Ӧƽ����ͼ�е�id

		bool       activeUsed{ false }; // �����֡���

		CellState  cellState{ CellState::InActive }; // ״̬

	public:
		inline RvtPageLevelTableCell() = default;

		inline RvtPageLevelTableCell(int x, int y, int width, int height, int mipLevel)
		: rect(x, y, width, height)
		, mipLevel(mipLevel) {}
	};

}