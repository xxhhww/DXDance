#pragma once
#include "WindowSetting.h"
#include "Tools/Event.h"
#include "EKey.h"
#include "EMouseButton.h"
#include <Windows.h>

namespace Windows {
	class Window {
	public:
		Window(const WindowSetting& setting = WindowSetting{});
	public:
		Tool::Event<EKey> keyPressedEvent;
		Tool::Event<EKey> keyReleasedEvent;
		Tool::Event<EMouseButton> mouseButtonPressedEvent;
		Tool::Event<EMouseButton> mouseButtonReleasedEvent;
		Tool::Event<int16_t, int16_t> mouseMoveEvent;
	private:
		static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
	private:
		std::string mTitle;
		int mWidth;
		int mHeight;
		HWND mHwnd{ nullptr };
	};
}