#include "ComboBox.h"

namespace UI {
	ComboBox::ComboBox(const std::string& label, int currentChoice)
	: IDataWidget<int>(currentChoice) 
	, mLabel(label) {}

	void ComboBox::_Draw_Internal_Impl() {
		if (choices.find(mData) == choices.end()) {
			mData = choices.begin()->first;
		}

		if (ImGui::BeginCombo((mLabel + mWidgetID).c_str(), choices[mData].c_str())) {
			for (const auto& [key, value] : choices) {
				bool selected = key == mData;
				if (ImGui::Selectable(value.c_str(), selected)) {
					if (!selected) {
						ImGui::SetItemDefaultFocus();
						mData = key;
					}
				}
			}
			ImGui::EndCombo();
		}
	}
}