#pragma once

namespace UI {
	/*
	* Interface to any plugin of OvUI.
	* A plugin is basically a behaviour that you can plug to a widget
	*/
	class IPlugin {
	public:
		/*
		* ÐéÎö¹¹
		*/
		virtual ~IPlugin() = default;

		/*
		* Execute the plugin behaviour
		*/
		virtual void Execute() = 0;
	};
}