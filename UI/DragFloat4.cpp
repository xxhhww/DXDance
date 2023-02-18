#include "DragFloat4.h"

namespace UI {
	DragFloat4::DragFloat4(
		const std::string& label,
		Math::Vector4 value,
		float min,
		float max,
		float speed,
		const std::string& format)
		: IDataWidget<Math::Vector4>(value)
		, label(label)
		, min(min)
		, max(max)
		, speed(speed)
		, format(format) {}

	void DragFloat4::_Draw_Internal_Impl() {
		if (max < min)
			max = min;
		if (min != max) {
			data = Math::Clamp(data, Math::Vector4(min), Math::Vector4(max));
		}

		ImGui::DragFloat4((label + mWidgetID).c_str(), reinterpret_cast<float*>(&data), speed, min, max, format.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(data);
		}
	}

	DragFloat4Split::DragFloat4Split(
		const std::string& label,
		Math::Vector4 value,
		Math::Vector4 min,
		Math::Vector4 max,
		float speed,
		const std::string& format)
		: IDataWidget<Math::Vector4>(value)
		, label(label)
		, min(min)
		, max(max)
		, speed(speed)
		, format(format) {}

	void DragFloat4Split::_Draw_Internal_Impl() {
		if (max < min)
			max = min;
		if (min != max) {
			data = Math::Clamp(data, Math::Vector4(min), Math::Vector4(max));
		}

		bool edited{ false };
		ImGui::DragFloat((label + mWidgetID + "_1").c_str(), &data.x, speed, min.x, max.x, format.c_str());
		edited = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::DragFloat((label + mWidgetID + "_2").c_str(), &data.y, speed, min.y, max.y, format.c_str());
		edited = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::DragFloat((label + mWidgetID + "_3").c_str(), &data.z, speed, min.z, max.z, format.c_str());
		edited = ImGui::IsItemDeactivatedAfterEdit();
		ImGui::DragFloat((label + mWidgetID + "_4").c_str(), &data.w, speed, min.w, max.w, format.c_str());
		edited = ImGui::IsItemDeactivatedAfterEdit();

		if (edited) {
			editCompletedEvent.Invoke(data);
		}
	}
}