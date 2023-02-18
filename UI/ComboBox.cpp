#include "ComboBox.h"

namespace UI {
	ComboBox::ComboBox(const std::string& label, int currentChoice)
	: IDataWidget<int>(currentChoice) 
	, mLabel(label) {}

	void ComboBox::_Draw_Internal_Impl() {
		if (choices.find(data) == choices.end()) {
			data = choices.begin()->first;
		}

		if (ImGui::BeginCombo((mLabel + mWidgetID).c_str(), choices[data].c_str())) {
			for (const auto& [key, value] : choices) {
				bool selected = key == data;
				if (ImGui::Selectable(value.c_str(), selected)) {
					if (!selected) {
						ImGui::SetItemDefaultFocus();
						data = key;
					}
				}
			}
			ImGui::EndCombo();
		}
	}
}