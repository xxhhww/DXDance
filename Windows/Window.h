#pragma once
#include "WindowSetting.h"
#include "Tools/Event.h"
#include "EKey.h"
#include "EMouseButton.h"
#include <Windows.h>
#include <optional>

namespace Windows {
	class Window {
	public:
		Window(const WindowSetting& setting = WindowSetting{});
		~Window();
	
		HWND GetHWND();
	private:
		static LRESULT CALLBACK HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		static LRESULT CALLBACK HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);
		LRESULT WINAPI HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

		static LRESULT CALLBACK WindowProc(HWND hWnd, UINT message, WPARAM wParam, LPARAM lParam);

	public:
		Tool::Event<EKey> keyPressedEvent;
		Tool::Event<EKey> keyReleasedEvent;
		Tool::Event<EMouseButton> mouseButtonPressedEvent;
		Tool::Event<EMouseButton> mouseButtonReleasedEvent;
		Tool::Event<int16_t, int16_t> mouseMoveEvent;
		Tool::Event<int32_t, int32_t> rawDeltaEvent;
		std::function<HRESULT(HWND, UINT, WPARAM, LPARAM)> imguiWndProcHandler;
	private:
		std::string mTitle;
		int32_t mWidth;
		int32_t mHeight;
		HWND mHwnd{ nullptr };
		WNDCLASSEXW mWc{ 0 };
	};
}