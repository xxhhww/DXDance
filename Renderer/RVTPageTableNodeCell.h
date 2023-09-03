#pragma once
#include "Renderer/RvtPagePayload.h"
#include "Math/Int.h"

namespace Renderer {

	struct RvtPageTableNodeCell {
	public:
		Math::Int4 rect;
		int mipLevel;
		RvtPagePayload payload;

	public:
		inline RvtPageTableNodeCell() = default;

		inline RvtPageTableNodeCell(int x, int y, int width, int height, int mipLevel)
		: rect(x, y, width, height)
		, mipLevel(mipLevel) {}

	};

}