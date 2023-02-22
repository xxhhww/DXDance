#include "Separator.h"
#include "imgui.h"

namespace UI {
	void Separator::_Draw_Internal_Impl() {
		ImGui::Separator();
	}
}