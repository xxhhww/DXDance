#include "DragFloat2.h"

namespace UI {
	DragFloat2::DragFloat2(
		const std::string& label,
		Math::Vector2 value,
		float min,
		float max,
		float speed,
		const std::string& format)
		: IDataWidget<Math::Vector2>(value)
		, label(label)
		, min(min)
		, max(max)
		, speed(speed)
		, format(format) {}

	void DragFloat2::_Draw_Internal_Impl() {
		if (max < min)
			max = min;
		data = Math::Clamp(data, Math::Vector2(min), Math::Vector2(max));

		ImGui::DragFloat2((label + mWidgetID).c_str(), reinterpret_cast<float*>(&data), speed, min, max, format.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(data);
		}
	}


	DragFloat2Split::DragFloat2Split(
		const std::string& label,
		Math::Vector2 value,
		Math::Vector2 min,
		Math::Vector2 max,
		float speed,
		const std::string& format) 
		: IDataWidget<Math::Vector2>(value)
		, label(label)
		, min(min)
		, max(max)
		, speed(speed)
		, format(format) {}

	void DragFloat2Split::_Draw_Internal_Impl() {
		if (max < min)
			max = min;
		data = Math::Clamp(data, Math::Vector2(min), Math::Vector2(max));

		bool edited{ false };
		ImGui::DragFloat((label + mWidgetID + "_1").c_str(), &data.x, speed, min.x, max.x, format.c_str());
		edited = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::DragFloat((label + mWidgetID + "_2").c_str(), &data.y, speed, min.y, max.y, format.c_str());
		edited = ImGui::IsItemDeactivatedAfterEdit();

		if (edited) {
			editCompletedEvent.Invoke(data);
		}
	}

}