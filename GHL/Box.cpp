#include "Box.h"

namespace GHL {
	
	Box::Box(
		uint32_t _left, uint32_t _right,
		uint32_t _top, uint32_t _bottom,
		uint32_t _front, uint32_t _back)
	: left(_left)
	, right(_right)
	, top(_top)
	, bottom(_bottom)
	, front(_front)
	, back(_back) {}

	D3D12_BOX Box::D3DBox() const {
		D3D12_BOX box{};
		box.left = left;
		box.right = right;
		box.top = top;
		box.bottom = bottom;
		box.front = front;
		box.back = back;

		return box;
	}

}