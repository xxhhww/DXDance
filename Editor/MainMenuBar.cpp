#include "MainMenuBar.h"

namespace App {
	/*
	* �˵����Ĺ��캯��
	*/
	MainMenuBar::MainMenuBar() {
		mWindowMenu = &CreateWidget<UI::MenuList>("Windows");
	}

	/*
	* �˵�������������
	*/
	MainMenuBar::~MainMenuBar() {
		mWindowMenu = nullptr;
	}

	/*
	* �򴰿ڲ˵���ע����Ҫ���Ƶ�PanelWindow�����
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