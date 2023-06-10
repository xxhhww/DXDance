#include "Rect.h"

namespace GHL {

	Rect::Rect(uint16_t topLeftX, uint16_t topLeftY, uint16_t width, uint16_t height) 
	: topLeftX(topLeftX)
	, topLeftY(topLeftY)
	, width(width)
	, height(height) {
	}

	Rect::Rect(uint16_t width, uint16_t height) 
	: topLeftX(0u)
	, topLeftY(0u)
	, width(width)
	, height(height) {
	}

	D3D12_RECT Rect::D3DRect() const {
		D3D12_RECT rect{};
		rect.left = topLeftX;
		rect.top = topLeftY;
		rect.right = topLeftX + width;
		rect.bottom = topLeftY + height;

		return rect;
	}

}