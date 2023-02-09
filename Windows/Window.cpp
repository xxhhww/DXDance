#include "Window.h"
#include "Tools/StrUtil.h"
#include "UI/imgui_impl_win32.h"
#include <assert.h>

using namespace Tool;

// Forward declare message handler from imgui_impl_win32.cpp
extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

namespace Windows {
	Window::Window(const WindowSetting& setting)
	: mTitle(setting.title)
	, mWidth(setting.width)
	, mHeight(setting.height) {
		// register window class
		WNDCLASSEXW wc = { sizeof(wc), CS_CLASSDC, &Window::HandleMsgSetup, 0L, 0L, GetModuleHandle(NULL), NULL, NULL, NULL, NULL, L"Window Class", NULL };
		::RegisterClassExW(&wc);
		// dwStyle
		DWORD dwStyle = 0;
		if (setting.visible) {
			dwStyle |= WS_VISIBLE;
		}
		// 全屏模式
		if (setting.fullscreen) {
			dwStyle |= WS_MAXIMIZE;
		}
		// 窗口模式
		else {
			dwStyle |= (WS_CAPTION | WS_SYSMENU);
			if (setting.maximized) {
				dwStyle |= WS_MAXIMIZE;
			}

			if (setting.resizable) {
				dwStyle |= (WS_MAXIMIZEBOX | WS_MINIMIZEBOX | WS_THICKFRAME);
			}
			else {
				dwStyle |= WS_BORDER;
			}
		}
		// create window
		mHwnd = ::CreateWindowW(
			wc.lpszClassName,
			StrUtil::ToWString(mTitle).c_str(),
			dwStyle,
			100, 100, mWidth, mHeight, 
			NULL, NULL, wc.hInstance, this);
		if (mHwnd == nullptr) {
			assert(false);
		}
		// Show the window
		::ShowWindow(mHwnd, SW_SHOWDEFAULT);
		::UpdateWindow(mHwnd);
	}

	LRESULT CALLBACK Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
		if (msg == WM_NCCREATE) {
			// extract ptr to window class from creation data
			const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
			Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
			// forward message to window instance handler
			return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
		}
		// if we get a message before the WM_NCCREATE message, handle with default handler
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	LRESULT WINAPI Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (ImGui_ImplWin32_WndProcHandler(hWnd, msg, wParam, lParam)) {
			return true;
		}

		switch (msg) {
		/* KeyBoard Message */
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
			keyPressedEvent.Invoke(static_cast<EKey>(wParam));
			break;
		case WM_KEYUP:
		case WM_SYSKEYUP:
			keyReleasedEvent.Invoke(static_cast<EKey>(wParam));
			break;
		// case WM_CHAR:
		/* MouseButton Message */
		case WM_MOUSEMOVE:
			const POINTS pt = MAKEPOINTS(lParam);
			mouseMoveEvent.Invoke(pt.x, pt.y);
			break;
		case WM_LBUTTONDOWN:
		case WM_RBUTTONDOWN:
		case WM_MBUTTONDOWN:
			mouseButtonPressedEvent.Invoke(static_cast<EMouseButton>(wParam));
			break;
		case WM_LBUTTONUP:
		case WM_RBUTTONUP:
		case WM_MBUTTONUP:
			mouseButtonReleasedEvent.Invoke(static_cast<EMouseButton>(wParam));
			break;
		default:
			return DefWindowProc(hWnd, msg, wParam, lParam);
		}
		return 0;
	}
}