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
		Tool::Event<const TData&> valueChangedEvent;
	protected:
		TData mData;
	};
}

#include "IDataWidget.inl"