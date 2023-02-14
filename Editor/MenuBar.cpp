#include "MenuBar.h"

namespace App {
	/*
	* 菜单栏的构造函数
	*/
	MenuBar::MenuBar() {
		mWindowMenu = &CreateWidget<UI::MenuList>("Windows");
	}

	/*
	* 菜单栏的析构函数
	*/
	MenuBar::~MenuBar() {
		mWindowMenu = nullptr;
	}

	/*
	* 向窗口菜单中注册需要控制的PanelWindow类对象
	*/
	void MenuBar::RegisterPanel(UI::PanelWindow* window) {
		mWindowMenu->CreateWidget<UI::MenuItem>(window->name, true, true).checkStatusChangedEvent += std::bind(&UI::PanelWindow::SetOpen, window, std::placeholders::_1);
	}
}