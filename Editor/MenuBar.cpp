#include "MenuBar.h"

namespace App {
	/*
	* �˵����Ĺ��캯��
	*/
	MenuBar::MenuBar() {
		mWindowMenu = &CreateWidget<UI::MenuList>("Windows");
	}

	/*
	* �˵�������������
	*/
	MenuBar::~MenuBar() {
		mWindowMenu = nullptr;
	}

	/*
	* �򴰿ڲ˵���ע����Ҫ���Ƶ�PanelWindow�����
	*/
	void MenuBar::RegisterPanel(UI::PanelWindow* window) {
		mWindowMenu->CreateWidget<UI::MenuItem>(window->name, true, true).checkStatusChangedEvent += std::bind(&UI::PanelWindow::SetOpen, window, std::placeholders::_1);
	}
}