#pragma once
#include "BrowserItemContextualMenu.h"
#include "Tools/Event.h"

namespace App {
	class FileItemContextualMenu : public BrowserItemContextualMenu {
	public:
		FileItemContextualMenu(const std::string& name, const std::string& path, bool isEngineItem);

		void BuildPopupContextItem() override;
	
	public:
		Tool::Event<>					itemDeledEvent;		// ����Ŀ��ɾ��
		Tool::Event<const std::string&>	itemRenamedEvent;	// ����Ŀ��������
	};
}