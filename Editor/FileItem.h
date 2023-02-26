#pragma once
#include "BrowserItem.h"

namespace App {
	class FileItem : public BrowserItem {
	public:
		FileItem(const std::string& name, const std::string& path);

		void PropagatePath(const std::string& parentPath) override;
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<> clickedEvent;
	};
}