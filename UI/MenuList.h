#pragma once
#include "IWidgetContainer.h"
#include "Tools/Event.h"

namespace UI {
	class MenuItem : public IWidget {
	public:
		MenuItem(const std::string& name, bool checkable = false, bool checkStatus = false);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<> clickedEvent;
		Tool::Event<bool> checkStatusChangedEvent;	// for mCheckStatus
	private:
		std::string mName;
		bool mCheckable;
		bool mCheckStatus;
	};

	class MenuList : public IWidget, public IWidgetContainer {
	public:
		MenuList(const std::string& name);

		inline auto IsOpen() const { return mOpenStatus; }
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<> clickedEvent;
	private:
		std::string mName;
		bool mOpenStatus{ false };
	};
}