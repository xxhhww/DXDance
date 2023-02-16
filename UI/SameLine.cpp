#include "SameLine.h"
#include "imgui.h"

namespace UI {
	void SameLine::_Draw_Internal_Impl() {
		ImGui::SameLine();
	}
}