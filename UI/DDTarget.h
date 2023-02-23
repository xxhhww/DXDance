#pragma once
#include "IPlugin.h"
#include "Tools/Event.h"
#include "imgui.h"
#include <string>

namespace UI {
	/*
	* Represents a drag and drop target
	*/
	template<typename T>
	class DDTarget : public IPlugin {
	public:
		/*
		* Create the drag and drop target
		*/
		DDTarget(const std::string& p_identifier);

		/*
		* Execute the drag and drop target behaviour
		*/
		void Execute() override;

		/*
		* Returns true if the drag and drop target is hovered by a drag and drop source
		*/
		bool IsHovered() const;

	public:
		std::string		identifier;
		Tool::Event<T>	dataReceivedEvent;
		Tool::Event<>	hoverStartEvent;
		Tool::Event<>	hoverEndEvent;

		bool showYellowRect = true;

	private:
		bool mIsHovered;
	};
}

#include "DDTarget.inl"