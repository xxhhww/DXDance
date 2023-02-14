#pragma once
#include "UI/PanelMenuBar.h"
#include "UI/PanelWindow.h"
#include "UI/MenuList.h"

namespace App {
	class MenuBar : public UI::PanelMenuBar {
	public:
		/*
		* �˵����Ĺ��캯��
		*/
		MenuBar();

		/*
		* �˵�������������
		*/
		~MenuBar();

		/*
		* �򴰿ڲ˵���ע����Ҫ���Ƶ�PanelWindow�����
		*/
		void RegisterPanel(UI::PanelWindow* window);
	private:

	private:
		UI::MenuList* mWindowMenu{ nullptr };	// ���ڲ˵������ƴ��ڵĿ���
	};
}