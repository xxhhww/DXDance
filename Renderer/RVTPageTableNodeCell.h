#pragma once
#include "Renderer/RVTPagePayload.h"
#include "Math/Int.h"

namespace Renderer {

	struct RVTPageTableNodeCell {
	public:
		Math::Int4 rect;
		int mipLevel;
		RVTPagePayload payload;

	public:
		inline RVTPageTableNodeCell() = default;

		inline RVTPageTableNodeCell(int x, int y, int width, int height, int mipLevel)
			: rect(x, y, width, height)
			, mipLevel(mipLevel) {}
	};

}