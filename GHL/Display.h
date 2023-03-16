#pragma once
#include "pbh.h"
#include "Math/Vector.h"

namespace GHL {
	/*
	* 显示器类
	*/
	class Display {
	public:
		/*
		* 构造函数
		*/
		Display(const DXGI_OUTPUT_DESC1& output);
		
		/*
		* 默认析构函数
		*/
		~Display() = default;

	public:
		ColorSpace colorSpace;
		Math::Vector2 redPrimary;
		Math::Vector2 greenPrimary;
		Math::Vector2 bluePrimary;
		Math::Vector2 whitePoint;
		float minLuminance{ 0.0f };
		float maxLuminance{ 1.0f };
		float maxFullFrameLuminance{ 1.0f };
		bool supportHDR{ false };
		Math::Vector2 rectOrigin;	// 原点
		Math::Vector2 rectSize;	// 长宽
	};
}