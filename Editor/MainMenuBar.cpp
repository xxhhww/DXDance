#include "MainMenuBar.h"

namespace App {
	/*
	* 菜单栏的构造函数
	*/
	MainMenuBar::MainMenuBar() {
		mWindowMenu = &CreateWidget<UI::MenuList>("Windows");
	}

	/*
	* 菜单栏的析构函数
	*/
	MainMenuBar::~MainMenuBar() {
		mWindowMenu = nullptr;
	}

	/*
	* 向窗口菜单中注册需要控制的PanelWindow类对象
	*/
	void MainMenuBar::RegisterPanel(UI::PanelWindow* window) {
		auto& menuItem = mWindowMenu->CreateWidget<UI::MenuItem>(window->name, true, true);
		menuItem.checkStatusChangedEvent += std::bind(&UI::PanelWindow::SetOpen, window, std::placeholders::_1);
		window->openedEvent += [&menuItem]() {
			menuItem.checkStatus = true;
		};
		window->closedEvent += [&menuItem]() {
			menuItem.checkStatus = false;
		};
	}
}