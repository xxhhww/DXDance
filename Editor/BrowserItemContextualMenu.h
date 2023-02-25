#pragma once
#include "UI/ContextualMenu.h"

namespace App {
	class BrowserItemContextualMenu : public UI::ContextualMenu {
	public:
		BrowserItemContextualMenu(const std::string& name, const std::string& path, bool isProtected);

		virtual void BuildPopupContextItem() = 0;
	public:
		std::string name;					// 项目名称
		std::string path;					// 项目绝对路径
		bool		isProtected{ false };	// 是否为引擎项目(引擎项目无法执行重命名、添加子项目、删除等操作)
	};
}