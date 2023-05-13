#include "Hierarchy.h"
#include "HierarchyContextualMenu.h"

#include "UI/TreeNode.h"

namespace App {
	Hierarchy::Hierarchy(
		const std::string& title,
		bool opened,
		const UI::PanelWindowSettings& panelSetting
	) 
	: PanelWindow(title, opened, panelSetting) {
		auto& testTree = CreateWidget<UI::TreeNode>("TTT");
		auto& contextualMenu = CreatePlugin<HierarchyContextualMenu>();
	}

	Hierarchy::~Hierarchy() {
	}

}