#include "TextSelectable.h"

namespace UI {
	TextSelectable::TextSelectable(const std::string& data)
	: IDataWidget<std::string>(data) {}

	void TextSelectable::_Draw_Internal_Impl() {
        bool useless = false;

        if (ImGui::Selectable((data + mWidgetID).c_str(), &useless, ImGuiSelectableFlags_AllowDoubleClick)) {
			clickedEvent.Invoke();
        }
	}
}