#pragma once
#include "DragSingleScalar.h"

namespace UI {
	template<typename TData>
	DragSingleScalar<TData>::DragSingleScalar(ImGuiDataType_ dataType, TData min, TData max, TData data, float speed,
		const std::string& label, const std::string& format, std::function<void(const TData&)>& dataChangedAction) 
	: IDataWidget<TData>(data, dataChangedAction) 
	, mDataType(dataType)
	, mMin(min) 
	, mMax(max) 
	, mSpeed(speed) 
	, mLabel(label) 
	, mFormat(format) {}

}