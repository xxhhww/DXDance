#include "InputFloat2.h"

namespace UI {
	InputFloat2::InputFloat2(
		Math::Vector2 value,
		float step,
		float stepFast,
		const std::string& label,
		const std::string& format)
		: IDataWidget<Math::Vector2>(value)
		, mStep(step)
		, mStepFast(stepFast)
		, mLabel(label)
		, mFormat(format) {}

	void InputFloat2::_Draw_Internal_Impl() {
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
		bool editCompleted = ImGui::InputScalarN((mLabel + mWidgetID).c_str(), ImGuiDataType_Float, reinterpret_cast<float*>(&data), 2, &mStep, &mStepFast, mFormat.c_str(), flags);
		if (editCompleted) {
			editCompletedEvent.Invoke(data);
		}
	}
}