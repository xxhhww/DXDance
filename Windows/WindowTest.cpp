#include "Window.h"
#include "InputManger.h"

using namespace Windows;

int WINAPI main(HINSTANCE hInstance, HINSTANCE, LPSTR, int nCmdShow) {
	WindowSetting setting{};
    setting.fullscreen = false;
    Window window{ setting };
    InputManger inputManger{ &window };

    bool done = false;
    while (!done)
    {
        // Poll and handle messages (inputs, window resize, etc.)
        // See the WndProc() function below for our to dispatch events to the Win32 backend.
        MSG msg;
        while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE))
        {
            if (msg.message == WM_QUIT) {
                done = true;
            }
            ::TranslateMessage(&msg);
            ::DispatchMessage(&msg);
        }
        if (done)
            break;
    }
}