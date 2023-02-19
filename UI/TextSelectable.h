#pragma once
#include "IDataWidget.h"

namespace UI {
	class TextSelectable : public IDataWidget<std::string> {
	public:
		TextSelectable(const std::string& data);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<> clickedEvent;
		Tool::Event<> doubleClickedEvent;
	};
}