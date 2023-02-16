#pragma once
#include "IDataWidget.h"
#include "imgui.h"

namespace UI {
	template<typename TData>
	IDataWidget<TData>::IDataWidget(const TData& data)
	: mData(data) {}

	template<typename TData>
	void IDataWidget<TData>::Draw() {
		if (mEnable) {
			TData prevData = mData;
			_Draw_Internal_Impl();
			if (prevData != mData) {
				valueChangedEvent.Invoke(mData);
			}
		}
	}
}