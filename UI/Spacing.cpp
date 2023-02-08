#include "Spacing.h"
#include "imgui.h"

namespace UI {
	Spacing::Spacing(uint32_t spacingNums)
	: mSpacings(spacingNums) {}
	
	void Spacing::_Draw_Internal_Impl() {
		for (uint32_t i = 0; i < mSpacings; i++) {
			ImGui::Spacing();
			if (i + 1 < mSpacings) {
				ImGui::SameLine();
			}
		}
	}
}