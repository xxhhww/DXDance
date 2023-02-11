#include "InputFloat3.h"

namespace UI {
	InputFloat3::InputFloat3(
		Math::Vector3 value,
		float step,
		float stepFast,
		const std::string& label,
		const std::string& format)
		: IDataWidget<Math::Vector3>(value)
		, mStep(step)
		, mStepFast(stepFast)
		, mLabel(label)
		, mFormat(format) {}

	void InputFloat3::_Draw_Internal_Impl() {
		ImGuiInputTextFlags flags = ImGuiInputTextFlags_EnterReturnsTrue;
		bool editCompleted = ImGui::InputScalarN((mLabel + mWidgetID).c_str(), ImGuiDataType_Float, reinterpret_cast<float*>(&mData), 3, &mStep, &mStepFast, mFormat.c_str(), flags);
		if (editCompleted) {
			editCompletedEvent.Invoke(mData);
		}
	}
}