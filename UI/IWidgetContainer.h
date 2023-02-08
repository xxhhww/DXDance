#pragma once
#include "IWidget.h"
#include <vector>

namespace UI {
	enum class IWidgetMangement {
		InternalMangement = 0,
		ExtraMangement	  = 1
	};

	class IWidgetContainer {
	public:
		void RegisterWidget(IWidget* widget);
		void UnregisterWidget(IWidget* widget);

		template <typename T, typename ...Args>
		T& CreateWidget(Args&&... args);

		void DeleteWidget(IWidget* widget);
		void DeleteAllWidgets();

		void DrawWidgets();
		void DoDestruction();
	protected:
		std::vector<std::pair<IWidget*, IWidgetMangement>> mWidgets;
	public:
		inline const auto& GetWidgets() const { return mWidgets; }
	};
}

#include "IWidgetContainer.inl"