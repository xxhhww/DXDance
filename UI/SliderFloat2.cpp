#include "SliderFloat2.h"

namespace UI {
	SliderFloat2::SliderFloat2(
		const std::string& label,
		Math::Vector2 value,
		float min,
		float max,
		const std::string& format)
		: IDataWidget<Math::Vector2>(value)
		, mLabel(label)
		, mMin(min)
		, mMax(max)
		, mFormat(format) {}

	void SliderFloat2::_Draw_Internal_Impl() {
		if (mMax < mMin)
			mMax = mMin;
		mData = Math::Clamp(mData, Math::Vector2(mMin), Math::Vector2(mMax));

		ImGui::SliderFloat2((mLabel + mWidgetID).c_str(), reinterpret_cast<float*>(&mData), mMin, mMax, mFormat.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(mData);
		}
	}
}