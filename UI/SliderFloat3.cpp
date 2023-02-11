#include "SliderFloat3.h"

namespace UI {
	SliderFloat3::SliderFloat3(
		const std::string& label,
		Math::Vector3 value,
		float min,
		float max,
		const std::string& format)
		: IDataWidget<Math::Vector3>(value)
		, mLabel(label)
		, mMin(min)
		, mMax(max)
		, mFormat(format) {}

	void SliderFloat3::_Draw_Internal_Impl() {
		if (mMax < mMin)
			mMax = mMin;
		mData = Math::Clamp(mData, Math::Vector3(mMin), Math::Vector3(mMax));

		ImGui::SliderFloat3((mLabel + mWidgetID).c_str(), reinterpret_cast<float*>(&mData), mMin, mMax, mFormat.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(mData);
		}
	}
}