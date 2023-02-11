#include "InputFloat.h"

namespace UI {
	InputFloat::InputFloat(
		float value,
		float step,
		float stepFast,
		const std::string& label,
		const std::string& format)
		: IDataWidget<float>(value)
		, mStep(step)
		, mStepFast(stepFast)
		, mLabel(label)
		, mFormat(format) {}

	void InputFloat::_Draw_Internal_Impl() {
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
		bool editCompleted = ImGui::InputFloat((mLabel + mWidgetID).c_str(), &mData, mStep, mStepFast, mFormat.c_str(), flags);
		if (editCompleted) {
			editCompletedEvent.Invoke(mData);
		}
	}
}