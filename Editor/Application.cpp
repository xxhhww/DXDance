#include "Application.h"

namespace App {
	/*
	* ��ʼ��������������༭��
	*/
	Application::Application(const std::string& projPath, const std::string& projName) 
	: mContext(projPath, projName)
	, mEditor(mContext) {}

	/*
	* ����Application
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

            // ���б༭��
            mEditor.Update(1.0f);
            mEditor.PostUpdate(1.0f);
        }
        return 0;
	}
}