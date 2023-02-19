#include "DragFloat3.h"

namespace UI {
	DragFloat3::DragFloat3(
		const std::string& label,
		Math::Vector3 value,
		float min,
		float max,
		float speed,
		const std::string& format)
		: IDataWidget<Math::Vector3>(value)
		, label(label)
		, min(min)
		, max(max)
		, speed(speed)
		, format(format) {}

	void DragFloat3::_Draw_Internal_Impl() {
		if (max < min)
			max = min;
		if (min != max) {
			data = Math::Clamp(data, Math::Vector3(min), Math::Vector3(max));
		}

		ImGui::DragFloat3((label + mWidgetID).c_str(), reinterpret_cast<float*>(&data), speed, min, max, format.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(data);
		}
	}

	DragFloat3Split::DragFloat3Split(
		const std::string& label,
		Math::Vector3 value,
		Math::Vector3 min,
		Math::Vector3 max,
		float speed,
		const std::string& format)
		: IDataWidget<Math::Vector3>(value)
		, label(label)
		, min(min)
		, max(max)
		, speed(speed)
		, format(format) {}

	void DragFloat3Split::_Draw_Internal_Impl() {
		if (max < min)
			max = min;
		if (min != max) {
			data = Math::Clamp(data, Math::Vector3(min), Math::Vector3(max));
		}

		bool edited{ false };
		ImGui::DragFloat((label + mWidgetID + "_1").c_str(), &data.x, speed, min.x, max.x, format.c_str());
		edited |= ImGui::IsItemDeactivatedAfterEdit();
		ImGui::DragFloat((label + mWidgetID + "_2").c_str(), &data.y, speed, min.y, max.y, format.c_str());
		edited |= ImGui::IsItemDeactivatedAfterEdit();
		ImGui::DragFloat((label + mWidgetID + "_3").c_str(), &data.z, speed, min.z, max.z, format.c_str());
		edited |= ImGui::IsItemDeactivatedAfterEdit();

		if (edited) {
			editCompletedEvent.Invoke(data);
		}
	}
}