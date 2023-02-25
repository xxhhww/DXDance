#include "BrowserItemContextualMenu.h"

namespace App {
	BrowserItemContextualMenu::BrowserItemContextualMenu(const std::string& name, const std::string& path, bool isProtected)
	: name(name)
	, path(path)
	, isProtected(isProtected) {}
}