#pragma once
#include "IDataWidget.h"

namespace UI {
	template<typename TData>
	IDataWidget<TData>::IDataWidget(const TData& data, std::function<void(const TData&)>& dataChangedAction)
		: mData(data)
		, mDataChangedAction(dataChangedAction) {}

	template<typename TData>
	void IDataWidget<TData>::Draw() {
		if (mEnable) {
			_Draw_Internal_Impl();
			if (!mLineBreak) {
				ImGui::SameLine();
			}
			if (mValueChanged) {
				mDataChangedAction(mData);
				mValueChanged = false;
			}
		}
	}

	template<typename TData>
	void IDataWidget<TData>::NotifyValueChanged() {
		mValueChanged = true;
	}
}