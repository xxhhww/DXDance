#include "DragFloat.h"

namespace UI {
	DragFloat::DragFloat(
		const std::string& label,
		float value,
		float min,
		float max,
		float speed,
		const std::string& format)
		: IDataWidget<float>(value)
		, label(label)
		, min(min)
		, max(max)
		, speed(speed)
		, format(format) {}

	void DragFloat::_Draw_Internal_Impl() {
		if (max < min)
			max = min;

		if (data < min)
			data = min;
		else if (data > max)
			data = max;

		ImGui::DragFloat((label + mWidgetID).c_str(), &data, speed, min, max, format.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(data);
		}
	}
}