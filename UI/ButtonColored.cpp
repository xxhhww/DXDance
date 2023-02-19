#include "ButtonColored.h"
#include "imgui.h"

namespace UI {
	ButtonColored::ButtonColored(
		const std::string& label,
		const Math::Color& color,
		const Math::Vector2& size,
		bool enableAlpha)
	: label(label)
	, color(color)
	, size(size)
	, enableAlpha(enableAlpha) {}

	void ButtonColored::_Draw_Internal_Impl() {
		ImVec4 targetColor{ color.x, color.y, color.z, 1.0f };

		if (ImGui::ColorButton((label + mWidgetID).c_str(), targetColor, !enableAlpha ? ImGuiColorEditFlags_NoAlpha : 0, ImVec2{ size.x, size.y })) {
			clickedEvent.Invoke();
		}
	}
}