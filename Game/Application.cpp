#include "Game/Application.h"

namespace Game {

	Application::Application(const std::string& name, HINSTANCE hInstance, int nCmdShow)
	: mContext(name, hInstance, nCmdShow) 
	, mGame(mContext) {
	}

	int Application::Run() {
        bool over = false;
        while (!over) {
            // Poll and handle messages (inputs, window resize, etc.)
            // See the WndProc() function below for our to dispatch events to the Win32 backend.
            MSG msg;
            while (::PeekMessage(&msg, NULL, 0U, 0U, PM_REMOVE)) {
                if (msg.message == WM_QUIT) {
                    over = true;
                }
                ::TranslateMessage(&msg);
                ::DispatchMessage(&msg);
            }
            if (over) break;

            // ÔËÐÐ±à¼­Æ÷
            mGame.Run();
        }
        return 0;
	}

}