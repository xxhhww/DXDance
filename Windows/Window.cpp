#include "Window.h"
#include "Tools/StrUtil.h"
#include <assert.h>

using namespace Tool;

namespace Windows {
	Window::Window(const WindowSetting& setting)
	: mTitle(setting.title)
	, mWidth(setting.width) 
	, mHeight(setting.height) {
		// register window class
		mWc.cbSize = sizeof(mWc);
		mWc.style = CS_HREDRAW | CS_VREDRAW;
		mWc.lpfnWndProc = &Window::HandleMsgSetup;
		mWc.cbClsExtra = 0;
		mWc.cbWndExtra = 0;
		mWc.hInstance = GetModuleHandle(NULL);
		mWc.hIcon = NULL;
		mWc.hCursor = NULL;
		mWc.hbrBackground = (HBRUSH)(GetStockObject(BLACK_BRUSH));
		mWc.lpszMenuName = NULL;
		mWc.lpszClassName = L"Window Class";
		mWc.hIconSm = NULL;
		::RegisterClassExW(&mWc);

		// dwStyle
		DWORD dwStyle = 0;	
		// 全屏模式
		if (setting.fullscreen) {
			dwStyle |= WS_POPUP;
			mWidth = GetSystemMetrics(SM_CXSCREEN);
			mHeight = GetSystemMetrics(SM_CYSCREEN);
		}
		// 窗口模式
		else {
			dwStyle |= (WS_CAPTION | WS_SYSMENU);
			if (setting.visible) {
				dwStyle |= WS_VISIBLE;
			}
			if (setting.maximized) {
				RECT rect{};
				SystemParametersInfo(SPI_GETWORKAREA, 0, &rect, 0);
				mWidth = rect.right - rect.left;
				mHeight = rect.bottom - rect.top;
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
		mHwnd = ::CreateWindow(
			mWc.lpszClassName, StrUtil::UTF8ToWString(mTitle).c_str(),
			dwStyle, 0, 0, mWidth, mHeight,
			NULL, NULL, mWc.hInstance, this);
		if (mHwnd == nullptr) {
			assert(false);
		}

		// register mouse raw input device
		RAWINPUTDEVICE rid{};
		rid.usUsagePage = 0x01; // mouse page
		rid.usUsage = 0x02;		// mouse usage
		rid.dwFlags = 0;
		rid.hwndTarget = mHwnd;
		assert(RegisterRawInputDevices(&rid, 1, sizeof(rid)));

		// Show the window
		if (setting.fullscreen) {
			::ShowWindow(mHwnd, SW_MAXIMIZE);
		}
		else {
			::ShowWindow(mHwnd, SW_SHOWDEFAULT);
		}
		::UpdateWindow(mHwnd);
	}

	Window::~Window() {
		::DestroyWindow(mHwnd);
		::UnregisterClassW(mWc.lpszClassName, mWc.hInstance);
	}

	HWND Window::GetHWND() {
		return mHwnd;
	}

	LRESULT CALLBACK Window::HandleMsgSetup(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// use create parameter passed in from CreateWindow() to store window class pointer at WinAPI side
		if (msg == WM_NCCREATE)
		{
			// extract ptr to window class from creation data
			const CREATESTRUCTW* const pCreate = reinterpret_cast<CREATESTRUCTW*>(lParam);
			Window* const pWnd = static_cast<Window*>(pCreate->lpCreateParams);
			// set WinAPI-managed user data to store ptr to window instance
			SetWindowLongPtr(hWnd, GWLP_USERDATA, reinterpret_cast<LONG_PTR>(pWnd));
			// set message proc to normal (non-setup) handler now that setup is finished
			SetWindowLongPtr(hWnd, GWLP_WNDPROC, reinterpret_cast<LONG_PTR>(&Window::HandleMsgThunk));
			// forward message to window instance handler
			return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
		}
		// if we get a message before the WM_NCCREATE message, handle with default handler
		return DefWindowProc(hWnd, msg, wParam, lParam);
	}

	LRESULT CALLBACK Window::HandleMsgThunk(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		// retrieve ptr to window instance
		Window* const pWnd = reinterpret_cast<Window*>(GetWindowLongPtr(hWnd, GWLP_USERDATA));
		// forward message to window instance handler
		return pWnd->HandleMsg(hWnd, msg, wParam, lParam);
	}

	LRESULT WINAPI Window::HandleMsg(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam) {
		if (imguiWndProcHandler != nullptr) {
			if (imguiWndProcHandler(hWnd, msg, wParam, lParam)) {
				return true;
			}
		}

		switch (msg) {
		/* KeyBoard Message */
		case WM_KEYDOWN:
		case WM_SYSKEYDOWN:
		{
			WORD vkCode = LOWORD(wParam);		// virtual-key code
			WORD keyFlags = HIWORD(lParam);
			WORD scanCode = LOBYTE(keyFlags);	// scan code
			BOOL isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED; // extended-key flag, 1 if scancode has 0xE0 prefix

			if (isExtendedKey) {
				scanCode = MAKEWORD(scanCode, 0xE0);
			}

			// if we want to distinguish these keys:
			switch (vkCode) {
			case VK_SHIFT:   // converts to VK_LSHIFT or VK_RSHIFT
			case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
			case VK_MENU:    // converts to VK_LMENU or VK_RMENU
				vkCode = LOWORD(MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX));
				break;
			default:
				vkCode = vkCode;
				break;
			}
			if (sEKeyMap.find(vkCode) != sEKeyMap.end()) {
				keyPressedEvent.Invoke(sEKeyMap.at(vkCode));
			}
			break;
		}
		/*
		case WM_CHAR:
		{
			if ((unsigned char)wParam >= 'a' && (unsigned char)wParam <= 'z') {
				wParam = std::toupper(wParam);
			}
			if (sEKeyMap.find((unsigned char)wParam) != sEKeyMap.end()) {
				keyPressedEvent.Invoke(sEKeyMap[wParam]);
			}
			break;
		}
		*/
		case WM_KEYUP:
		case WM_SYSKEYUP:
		{
			WORD vkCode = LOWORD(wParam);		// virtual-key code
			WORD keyFlags = HIWORD(lParam);
			WORD scanCode = LOBYTE(keyFlags);	// scan code
			BOOL isExtendedKey = (keyFlags & KF_EXTENDED) == KF_EXTENDED; // extended-key flag, 1 if scancode has 0xE0 prefix

			if (isExtendedKey) {
				scanCode = MAKEWORD(scanCode, 0xE0);
			}

			// if we want to distinguish these keys:
			switch (vkCode) {
			case VK_SHIFT:   // converts to VK_LSHIFT or VK_RSHIFT
			case VK_CONTROL: // converts to VK_LCONTROL or VK_RCONTROL
			case VK_MENU:    // converts to VK_LMENU or VK_RMENU
				vkCode = LOWORD(MapVirtualKeyW(scanCode, MAPVK_VSC_TO_VK_EX));
				break;
			default:
				vkCode = vkCode;
				break;
			}
			if (sEKeyMap.find(vkCode) != sEKeyMap.end()) {
				keyReleasedEvent.Invoke(sEKeyMap.at(vkCode));
			}
			break;
		}
		/* MouseButton Message */
		case WM_MOUSEMOVE:
		{
			const POINTS pt = MAKEPOINTS(lParam);
			mouseMoveEvent.Invoke(pt.x, pt.y);
			break;
		}
		case WM_LBUTTONDOWN:
			mouseButtonPressedEvent.Invoke(EMouseButton::MOUSE_LBUTTON);
			break;
		case WM_RBUTTONDOWN:
			mouseButtonPressedEvent.Invoke(EMouseButton::MOUSE_RBUTTON);
			break;
		case WM_MBUTTONDOWN:
			mouseButtonPressedEvent.Invoke(EMouseButton::MOUSE_MBUTTON);
			break;
		case WM_LBUTTONUP:
			mouseButtonReleasedEvent.Invoke(EMouseButton::MOUSE_LBUTTON);
			break;
		case WM_RBUTTONUP:
			mouseButtonReleasedEvent.Invoke(EMouseButton::MOUSE_RBUTTON);
			break;
		case WM_MBUTTONUP:
			mouseButtonReleasedEvent.Invoke(EMouseButton::MOUSE_MBUTTON);
			break;
		case WM_INPUT:
		{
			UINT size = 0u;
			std::vector<BYTE> rawDatas;
			// first get the size of the input data
			if (GetRawInputData(
				reinterpret_cast<HRAWINPUT>(lParam),
				RID_INPUT,
				nullptr,
				&size,
				sizeof(RAWINPUTHEADER)) == -1) {
				// bail msg processing if error
				break;
			}
			rawDatas.resize(size);
			// read in the input data
			if (GetRawInputData(
				reinterpret_cast<HRAWINPUT>(lParam),
				RID_INPUT,
				rawDatas.data(),
				&size,
				sizeof(RAWINPUTHEADER)) != size) {
				// bail msg processing if error
				break;
			}
			// process the raw input data
			auto& ri = reinterpret_cast<const RAWINPUT&>(*rawDatas.data());
			if (ri.header.dwType == RIM_TYPEMOUSE) {
				rawDeltaEvent.Invoke(ri.data.mouse.lLastX, ri.data.mouse.lLastY);
			}
			break;
		}
		case WM_DESTROY:
		{
			::PostQuitMessage(0);
			break;
		}
		default:
			return ::DefWindowProc(hWnd, msg, wParam, lParam);
		}
		return 0;
	}
}