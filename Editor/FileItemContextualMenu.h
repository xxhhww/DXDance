#pragma once
#include "BrowserItemContextualMenu.h"
#include "Tools/Event.h"

namespace App {
	class FileItemContextualMenu : public BrowserItemContextualMenu {
	public:
		FileItemContextualMenu(const std::string& name, const std::string& path, bool isEngineItem);

		void BuildPopupContextItem() override;
	
	public:
		Tool::Event<>					itemDeledEvent;		// 此项目被删除
		Tool::Event<const std::string&>	itemRenamedEvent;	// 此项目被重命名
	};
}