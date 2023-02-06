#pragma once
#include "IDataWidget.h"
#include "imgui.h"

namespace UI {
	template<typename TData>
	class DragSingleScalar : public IDataWidget<TData> {
	public:
		DragSingleScalar(ImGuiDataType_ dataType, TData min, TData max, TData data, float speed,
			const std::string& label, const std::string& format, std::function<void(const TData&)>& dataChangedAction);
	protected:
		using IDataWidget<TData>::mData;

		inline void _Draw_Internal_Impl() override {
			if (mMax < mMin)
				mMax = mMin;

			if (mData < mMin)
				mData = mMin;
			else if (mData > mMax)
				mData = mMax;

			if(ImGui::DragScalar((mLabel + this->mWidgetID).c_str(), mDataType, &mData, mSpeed, &mMin, &mMax, mFormat.c_str()) ){
				this->NotifyValueChanged();
			}
		}
	private:
		ImGuiDataType_ mDataType;
		TData mMin;
		TData mMax;
		float mSpeed;
		std::string mLabel;
		std::string mFormat;
	};
}

#include "DragSingleScalar.inl"