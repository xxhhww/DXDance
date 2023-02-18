#include "InputFloat4.h"

namespace UI {
	InputFloat4::InputFloat4(
		Math::Vector4 value,
		float step,
		float stepFast,
		const std::string& label,
		const std::string& format)
		: IDataWidget<Math::Vector4>(value)
		, mStep(step)
		, mStepFast(stepFast)
		, mLabel(label)
		, mFormat(format) {}

	void InputFloat4::_Draw_Internal_Impl() {
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
		bool editCompleted = ImGui::InputScalarN((mLabel + mWidgetID).c_str(), ImGuiDataType_Float, reinterpret_cast<float*>(&data), 4, &mStep, &mStepFast, mFormat.c_str(), flags);
		if (editCompleted) {
			editCompletedEvent.Invoke(data);
		}
	}
}