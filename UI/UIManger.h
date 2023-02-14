#pragma once
#include "Canvas.h"
#include "Windows/Window.h"
#include "Setting.h"

namespace UI {
	class UIManger {
	public:
		/*
		* ImGUi与Imnodes的初始化，依赖于窗口类与D3D相关类(设备、描述符堆、描述符等)
		*/
		UIManger(Windows::Window* window, UIStyle style);

		/*
		* ImGui与Imnodes的析构，画布(Canvas)的析构由Editor类完成
		*/
		~UIManger();

		/*
		* 应用对应的ImGui风格
		*/
		void ApplyStyle(UIStyle style);

		/*
		* 设置UIManger的画布，由Editor类进行设置
		*/
		void SetCanvas(Canvas* canvas);

		/*
		* 绘制画布
		*/
		void Darw();
	private:
		Canvas* mCanvas{ nullptr };
		UIStyle mStyle;
	};
}