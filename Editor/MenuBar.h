#pragma once
#include "UI/PanelMenuBar.h"
#include "UI/PanelWindow.h"
#include "UI/MenuList.h"

namespace App {
	class MenuBar : public UI::PanelMenuBar {
	public:
		/*
		* 菜单栏的构造函数
		*/
		MenuBar();

		/*
		* 菜单栏的析构函数
		*/
		~MenuBar();

		/*
		* 向窗口菜单中注册需要控制的PanelWindow类对象
		*/
		void RegisterPanel(UI::PanelWindow* window);
	private:

	private:
		UI::MenuList* mWindowMenu{ nullptr };	// 窗口菜单，控制窗口的开关
	};
}