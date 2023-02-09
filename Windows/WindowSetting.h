#pragma once
#include <string>
#include "ECursor.h"

namespace Windows {
	struct WindowSetting {
		/*
		* Title of the window (Displayed in the title bar)
		*/
		std::string title;

		/*
		* Width in pixels of the window
		*/
		uint16_t width;

		/*
		* Height in pixels of the window
		*/
		uint16_t height;

		/*
		* 指定窗口是否为全屏模式(无边框、标题栏、不可缩放)或者窗口模式(有边框、有标题拦)
		*/
		bool fullscreen = false;

		/*
		* 指定窗口是否可缩放，只在窗口模式时有效
		*/
		bool resizable = true;

		/*
		* 指定窗口创建时是否最大化，只在窗口模式时有效
		*/
		bool maximized = true;

		/*
		* 指定窗口窗口时是否可见
		*/
		bool visible = true;
	};
}