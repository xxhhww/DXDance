#pragma once
#include "IPlugin.h"
#include "IWidgetContainer.h"

namespace UI {
	/*
	* A simple plugin that will show a contextual menu on right click
	* You can add widgets to a contextual menu
	*/
	class ContextualMenu : public IPlugin, public IWidgetContainer {
	public:
		/*
		* Execute the plugin
		*/
		void Execute();

		/*
		* Force close the contextual menu
		*/
		void Close();
	};
}