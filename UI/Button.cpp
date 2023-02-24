#include "Button.h"
#include "imgui.h"
#include "imgui_internal.h"

namespace UI {
	Button::Button(const std::string& label, const Math::Vector2& size, bool disable) 
	: label(label) 
	, size(size) 
	, disable(disable) {}

	void Button::_Draw_Internal_Impl() {
		ImGui::ButtonEx((label + mWidgetID).c_str(), ImVec2(size.x, size.y), disable ? ImGuiButtonFlags_Disabled : 0);

		if (ImGui::IsItemClicked() && ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
			doubleClickedEvent.Invoke();
		}
		if (ImGui::IsItemClicked() && ImGui::IsMouseClicked(ImGuiMouseButton_Left)) {
			clickedEvent.Invoke();
		}
	}

}