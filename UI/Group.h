#pragma once
#include "IWidgetContainer.h"
#include "Tools/Event.h"

namespace UI {
	class Group : public IWidget, public IWidgetContainer {
	public:
	protected:
		virtual void _Draw_Internal_Impl() override;
	};

	class GroupCollapsable : public Group {
	public:
		GroupCollapsable(const std::string& name);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<> clickedEvent;
		Tool::Event<> doubleClickedEvent;
		Tool::Event<> openedEvent;
		Tool::Event<> closedEvent;
	private:
		std::string mName;
		bool mOpenStatus{ false };
	};
}