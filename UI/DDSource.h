#pragma once
#include "IPlugin.h"
#include "Tools/Event.h"
#include "imgui.h"
#include <string>

namespace UI {
	/*
	* Represents a drag and drop source
	*/
	template<typename T>
	class DDSource : public IPlugin {
	public:
		/*
		* Create the drag and drop source
		*/
		DDSource
		(
			const std::string& p_identifier,
			const std::string& p_tooltip,
			T p_data
		);

		/*
		* Execute the behaviour of the drag and drop source
		*/
		void Execute() override;

		/*
		* Returns true if the drag and drop source is dragged
		*/
		bool IsDragged() const;

	public:
		std::string identifier;
		std::string tooltip;
		T data;
		Tool::Event<> dragStartEvent;
		Tool::Event<> dragStopEvent;

		bool hasTooltip = true;

	private:
		bool mIsDragged;
	};
}

#include "DDSource.inl"