#pragma once
#include "IPlugin.h"
#include "IWidgetContainer.h"

namespace UI {
	/*
	* A simple plugin that will show a contextual menu on right click
	* You can add widgets to a contextual menu
	*/
	enum class ContextualMenuType {
		Window,
		Item
	};

	class ContextualMenu : public IPlugin, public IWidgetContainer {
	public:
		ContextualMenu(ContextualMenuType menuType = ContextualMenuType::Item);

		/*
		* Execute the plugin
		*/
		void Execute();

		/*
		* Force close the contextual menu
		*/
		void Close();

	protected:
		ContextualMenuType mMenuType;
	};
}