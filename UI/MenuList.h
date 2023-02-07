#pragma once
#include "IWidgetContainer.h"
#include "Tools/Event.h"

namespace UI {
	class MenuItem : public IWidget {
	public:
		MenuItem(const std::string& name, bool clickable);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<> clickedEvent;
	private:
		std::string mName;
		bool mClickable;
		bool mIsClicked{ false };
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