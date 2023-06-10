#pragma once
#include "pbh.h"

namespace GHL {

	class Rect {
	public:
		Rect(uint16_t topLeftX, uint16_t topLeftY, uint16_t width, uint16_t height);
		Rect(uint16_t width, uint16_t height);

		D3D12_RECT D3DRect() const;

	public:
		uint16_t topLeftX;
		uint16_t topLeftY;
		uint16_t width;
		uint16_t height;
	};

}