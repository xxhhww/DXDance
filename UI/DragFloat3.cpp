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
		, mLabel(label)
		, mMin(min)
		, mMax(max)
		, mSpeed(speed)
		, mFormat(format) {}

	void DragFloat3::_Draw_Internal_Impl() {
		if (mMax < mMin)
			mMax = mMin;
		mData = Math::Clamp(mData, Math::Vector3(mMin), Math::Vector3(mMax));

		ImGui::DragFloat3((mLabel + mWidgetID).c_str(), reinterpret_cast<float*>(&mData), mSpeed, mMin, mMax, mFormat.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(mData);
		}
	}
}