#include "HierarchyContextualMenu.h"

#include "UI/MenuList.h"
#include "UI/InputText.h"

namespace App {

	HierarchyContextualMenu::HierarchyContextualMenu() {
		BuildPopupContextItem();
	}

	void HierarchyContextualMenu::BuildPopupContextItem() {

		// 一级目录
		auto& createMenu = CreateWidget<UI::MenuList>("Create");
		auto& renameMenu = CreateWidget<UI::MenuList>("Rename");
		auto& deleteMenu = CreateWidget<UI::MenuItem>("Delete");

		// 二级目录
		auto& createSphereItem = createMenu.CreateWidget<UI::MenuItem>("Sphere");

		auto& renameInputTextItem = renameMenu.CreateWidget<UI::InputText>("##hidelabel", "");

		// 
	}

}