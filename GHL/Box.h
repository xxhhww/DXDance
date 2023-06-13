#pragma once
#include "pbh.h"

namespace GHL {

	class Box {
	public:
		Box(uint32_t _left, uint32_t _right, 
			uint32_t _top, uint32_t _bottom, 
			uint32_t _front, uint32_t _back);

		D3D12_BOX D3DBox() const;

	public:
		uint32_t left;
		uint32_t right;
		uint32_t top;
		uint32_t bottom;
		uint32_t front;
		uint32_t back;
	};

}