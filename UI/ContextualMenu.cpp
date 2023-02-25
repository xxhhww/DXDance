#include "ContextualMenu.h"
#include "imgui.h"

namespace UI {
	void ContextualMenu::Execute() {
		if (ImGui::BeginPopupContextItem()) {
			DrawWidgets();
			ImGui::EndPopup();
		}
	}

	void ContextualMenu::Close() {
		ImGui::CloseCurrentPopup();
	}
}