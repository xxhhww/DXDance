#pragma once
#include "IDataWidget.h"
#include "Tools/Event.h"

namespace UI {
	class InputText : public IDataWidget<std::string> {
	public:
		InputText(const std::string& content, const std::string& label, size_t maxSize = 256);
	public:
		Tool::Event<const std::string&> editCompletedEvent;
	protected:
		void _Draw_Internal_Impl() override;
	private:
		std::string mLabel;
		size_t mMaxSize;
	};
}