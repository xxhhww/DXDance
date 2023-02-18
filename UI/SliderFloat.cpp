#include "SliderFloat.h"

namespace UI {
	SliderFloat::SliderFloat(
		const std::string& label,
		float value,
		float min,
		float max,
		const std::string& format)
		: IDataWidget<float>(value)
		, mLabel(label)
		, mMin(min)
		, mMax(max)
		, mFormat(format) {}

	void SliderFloat::_Draw_Internal_Impl() {
		if (mMax < mMin)
			mMax = mMin;

		if (data < mMin)
			data = mMin;
		else if (data > mMax)
			data = mMax;

		ImGui::SliderFloat((mLabel + mWidgetID).c_str(), &data, mMin, mMax, mFormat.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(data);
		}
	}
}