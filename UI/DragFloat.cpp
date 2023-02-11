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
		, mLabel(label)
		, mMin(min)
		, mMax(max)
		, mSpeed(speed)
		, mFormat(format) {}

	void DragFloat::_Draw_Internal_Impl() {
		if (mMax < mMin)
			mMax = mMin;

		if (mData < mMin)
			mData = mMin;
		else if (mData > mMax)
			mData = mMax;

		ImGui::DragFloat((mLabel + mWidgetID).c_str(), &mData, mSpeed, mMin, mMax, mFormat.c_str());
		if (ImGui::IsItemDeactivatedAfterEdit()) {
			// Item ±»±à¼­Íê³É
			editCompletedEvent.Invoke(mData);
		}
	}
}