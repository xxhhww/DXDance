#pragma once
#include "IWidget.h"
#include <functional>

namespace UI {
	template<typename TData>
	class IDataWidget : public IWidget {
	public:
		IDataWidget(const TData& data, std::function<void(const TData&)>& dataChangedAction);
		virtual ~IDataWidget() = default;

		void Draw() override;
		void NotifyValueChanged();
	protected:
		TData mData;
		bool mValueChanged{ false };
		std::function<void(const TData&)> mDataChangedAction{ nullptr };
	public:
		inline TData& GetData() { return mData; }
	};
}

#include "IDataWidget.inl"