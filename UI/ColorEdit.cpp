#include "ColorEdit.h"

namespace UI {
	ColorEdit::ColorEdit(const std::string& label, Math::Color value)
	: IDataWidget<Math::Color>(value) 
	, mLabel(label) {}

	void ColorEdit::_Draw_Internal_Impl() {
		ImGui::ColorEdit3((mLabel + mWidgetID).c_str(), reinterpret_cast<float*>(&data));
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			editCompletedEvent.Invoke(data);
		}
	}
}