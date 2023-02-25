#include "BrowserItem.h"

namespace App {
	BrowserItem::BrowserItem(const std::string& name, const std::string& path, bool isDirectory)
	: name(name)
	, path(path) 
	, isDirectory(isDirectory) {}
}