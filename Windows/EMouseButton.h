#pragma once
#include <Windows.h>
#include <unordered_map>

namespace Windows {
	enum class EMouseButton : int {
		MOUSE_LBUTTON,
		MOUSE_RBUTTON,
		MOUSE_MBUTTON,
	};

	static std::unordered_map<int, EMouseButton> sEMouseButtonMap =
	{
		{ VK_LBUTTON, EMouseButton::MOUSE_LBUTTON },
		{ VK_RBUTTON, EMouseButton::MOUSE_RBUTTON },
		{ VK_MBUTTON, EMouseButton::MOUSE_MBUTTON },
	};
}