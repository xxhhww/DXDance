#pragma once
#include "IDataWidget.h"


namespace UI {
	class InputTextMultiline : public IDataWidget<std::string> {
	public:
		InputTextMultiline(const std::string& label, const std::string& content, size_t maxSize = 1024 * 16);
	public:
		Tool::Event<const std::string&> editCompletedEvent;
	protected:
		void _Draw_Internal_Impl() override;
	private:
		std::string mLabel;
		size_t mMaxSize;
	};
}