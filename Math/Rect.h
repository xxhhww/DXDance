#pragma once

namespace Math {

	struct Rect {
	public:
		Rect(int x, int y, int width, int height);
		~Rect() = default;

		int X;
		int Y;
		int Width;
		int Height;
	};

}