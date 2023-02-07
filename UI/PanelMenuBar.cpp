#include "PanelMenuBar.h"
#include "imgui.h"

namespace UI {
	void PanelMenuBar::_Draw_Internal_Impl() {
		if (!mWidgets.empty() && ImGui::BeginMainMenuBar()) {
			DrawWidgets();
			ImGui::EndMainMenuBar();
		}
	}
}