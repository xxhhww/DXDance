#include "FileItemContextualMenu.h"

namespace App {
	FileItemContextualMenu::FileItemContextualMenu(const std::string& name, const std::string& path, bool isEngineItem)
	: BrowserItemContextualMenu(name, path, isEngineItem) {}

	void FileItemContextualMenu::BuildPopupContextItem() {

	}
}