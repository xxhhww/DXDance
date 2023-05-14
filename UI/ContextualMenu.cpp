#include "ContextualMenu.h"
#include "imgui.h"

namespace UI {
	ContextualMenu::ContextualMenu(ContextualMenuType menuType)
	: mMenuType(menuType) {}

	void ContextualMenu::Execute() {
		if (mMenuType == ContextualMenuType::Item) {
			if (ImGui::BeginPopupContextItem()) {
				DrawWidgets();
				ImGui::EndPopup();
			}
		}
		else if (mMenuType == ContextualMenuType::Window) {
			if (ImGui::BeginPopupContextWindow(nullptr, 1, false)) {
				DrawWidgets();
				ImGui::EndPopup();
			}
		}
	}

	void ContextualMenu::Close() {
		ImGui::CloseCurrentPopup();
	}
}