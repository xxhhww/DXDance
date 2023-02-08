#include "InputTextMultiline.h"

namespace UI {
	InputTextMultiline::InputTextMultiline(const std::string& label, const std::string& content, size_t maxSize)
	: IDataWidget<std::string>(content)
	, mLabel(label)
	, mMaxSize(maxSize) {}

	void InputTextMultiline::_Draw_Internal_Impl() {
		bool editCompleted = ImGui::InputText((mLabel + mWidgetID).c_str(), &mData[0], mMaxSize, ImGuiInputTextFlags_EnterReturnsTrue);
		if (editCompleted) {
			editCompletedEvent.Invoke(mData);
		}
	}
}