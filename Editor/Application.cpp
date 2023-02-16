#include "Application.h"

namespace App {
	/*
	* 初始化引擎上下文与编辑器
	*/
	Application::Application(const std::string& projPath, const std::string& projName) 
	: mContext(projPath, projName)
	, mEditor(mContext) {}

	/*
	* 运行Application
	*/
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

            // 运行编辑器
            mEditor.Update(1.0f);
            mEditor.PostUpdate(1.0f);
        }
        return 0;
	}
}