#pragma once
#include "IWidget.h"
#include "Tools/Event.h"

namespace UI {
	template<typename TData>
	class IDataWidget : public IWidget {
	public:
		IDataWidget(const TData& data);
		virtual ~IDataWidget() = default;

		void Draw() override;
	public:
		std::function<TData(void)> dataGatherer{ nullptr };
		std::function<void(TData)> dataProvider{ nullptr };

		Tool::Event<const TData&> valueChangedEvent;
		TData data;
	};
}

#include "IDataWidget.inl"