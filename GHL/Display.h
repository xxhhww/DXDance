#pragma once
#include "pbh.h"
#include "Math/Vector.h"

namespace GHL {
	/*
	* ��ʾ����
	*/
	class Display {
	public:
		/*
		* ���캯��
		*/
		Display(const DXGI_OUTPUT_DESC1& output);
		
		/*
		* Ĭ����������
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
		Math::Vector2 rectOrigin;	// ԭ��
		Math::Vector2 rectSize;	// ����
	};
}