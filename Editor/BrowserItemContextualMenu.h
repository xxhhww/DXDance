#pragma once
#include "UI/ContextualMenu.h"

namespace App {
	class BrowserItemContextualMenu : public UI::ContextualMenu {
	public:
		BrowserItemContextualMenu(const std::string& name, const std::string& path, bool isProtected);

		virtual void BuildPopupContextItem() = 0;
	public:
		std::string name;					// ��Ŀ����
		std::string path;					// ��Ŀ����·��
		bool		isProtected{ false };	// �Ƿ�Ϊ������Ŀ(������Ŀ�޷�ִ�����������������Ŀ��ɾ���Ȳ���)
	};
}