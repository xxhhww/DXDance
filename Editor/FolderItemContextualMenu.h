#pragma once
#include "BrowserItemContextualMenu.h"
#include "Tools/Event.h"

namespace App {
	class FolderItemContextualMenu : public BrowserItemContextualMenu {
	public:
		FolderItemContextualMenu(const std::string& name, const std::string& path, bool isEngineItem);

		void BuildPopupContextItem() override;
	public:
		Tool::Event<const std::string&> itemAddedEvent;	// ����Ŀ�����
		Tool::Event<const std::string&> itemDeledEvent;	// ����Ŀ��ɾ��
	};
}