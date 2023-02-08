#include "NewLine.h"
#include "imgui.h"

namespace UI {
	void NewLine::_Draw_Internal_Impl() {
		ImGui::NewLine();
	}
}