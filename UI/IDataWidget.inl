#pragma once
#include "IDataWidget.h"
#include "imgui.h"

namespace UI {
	template<typename TData>
	IDataWidget<TData>::IDataWidget(const TData& data)
	: data(data) {
		mAutoExecutePlugins = false;
	}

	template<typename TData>
	void IDataWidget<TData>::Draw() {
		if (mEnable) {

			if (dataGatherer != nullptr) {
				data = dataGatherer();
			}

			TData prevData = data;
			_Draw_Internal_Impl();

			if (dataProvider != nullptr) {
				dataProvider(data);
			}

			if (prevData != data) {
				valueChangedEvent.Invoke(data);
			}

			if (mAutoExecutePlugins) {
				ExecuteAllPlugins();
			}
		}
	}
}