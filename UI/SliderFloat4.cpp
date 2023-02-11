#include "SliderFloat4.h"

namespace UI {
	SliderFloat4::SliderFloat4(
		const std::string& label,
		Math::Vector4 value,
		float min,
		float max,
		const std::string& format)
		: IDataWidget<Math::Vector4>(value)
		, mLabel(label)
		, mMin(min)
		, mMax(max)
		, mFormat(format) {}

	void SliderFloat4::_Draw_Internal_Impl() {
		if (mMax < mMin)
			mMax = mMin;
		mData = Math::Clamp(mData, Math::Vector4(mMin), Math::Vector4(mMax));

		ImGui::SliderFloat4((mLabel + mWidgetID).c_str(), reinterpret_cast<float*>(&mData), mMin, mMax, mFormat.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(mData);
		}
	}
}