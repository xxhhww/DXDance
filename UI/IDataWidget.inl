#pragma once
#include "IDataWidget.h"
#include "imgui.h"

namespace UI {
	template<typename TData>
	IDataWidget<TData>::IDataWidget(const TData& data)
	: data(data) {}

	template<typename TData>
	void IDataWidget<TData>::Draw() {
		if (mEnable) {
			TData prevData = data;
			_Draw_Internal_Impl();
			if (mAutoExecutePlugins) {
				ExecuteAllPlugins();
			}
			if (prevData != data) {
				valueChangedEvent.Invoke(data);
			}
		}
	}
}