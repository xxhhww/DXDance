#pragma once
#include "IDataWidget.h"

namespace UI {
	class Text : public IDataWidget<std::string> {
	public:
		Text(const std::string& content);

	protected:
		void _Draw_Internal_Impl() override;
	};
}