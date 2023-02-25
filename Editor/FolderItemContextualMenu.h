#pragma once
#include "BrowserItemContextualMenu.h"
#include "Tools/Event.h"

namespace App {
	class FolderItemContextualMenu : public BrowserItemContextualMenu {
	public:
		FolderItemContextualMenu(const std::string& name, const std::string& path, bool isEngineItem);

		void BuildPopupContextItem() override;
	public:
		Tool::Event<const std::string&> itemAddedEvent;	// 新项目被添加
		Tool::Event<const std::string&> itemDeledEvent;	// 旧项目被删除
	};
}