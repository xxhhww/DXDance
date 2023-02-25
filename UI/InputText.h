#pragma once
#include "IDataWidget.h"
#include "Tools/Event.h"

namespace UI {
	class InputText : public IDataWidget<std::string> {
	public:
		InputText(const std::string& label, const std::string& content, size_t maxSize = 256);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<const std::string&> enterPressedEvent;
	private:
		std::string mLabel;
		size_t mMaxSize;
	};
}