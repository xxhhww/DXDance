#pragma once
#include "BrowserItemContextualMenu.h"

namespace App {
	class FileItemContextualMenu : public BrowserItemContextualMenu {
	public:
		FileItemContextualMenu(const std::string& name, const std::string& path, bool isEngineItem);

		void BuildPopupContextItem() override;
	};
}