#pragma once
#include "BrowserItemContextualMenu.h"
#include "Tools/Event.h"

namespace App {
	class FolderItem;

	class FolderItemContextualMenu : public BrowserItemContextualMenu {
	public:
		FolderItemContextualMenu(const std::string& name, const std::string& path, bool isEngineItem);

		void BuildPopupContextItem() override;
	public:
		Tool::Event<const std::string&> itemAddedEvent;		// ����Ŀ�����
		Tool::Event<>					itemDeledEvent;		// ����Ŀ��ɾ��
		Tool::Event<const std::string&>	itemRenamedEvent;	// ����Ŀ��������
	};
}