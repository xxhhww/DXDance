#pragma once
#include "IDataWidget.h"
#include "Math/Color.h"

namespace UI {
	class ColorEdit : public IDataWidget<Math::Color> {
	public:
		ColorEdit(const std::string& label, Math::Color value = Math::Color{ 0.0f, 0.0f, 0.0f });
	protected:
		void _Draw_Internal_Impl() override;
	public:
		Tool::Event<Math::Color> editCompletedEvent;
	private:
		std::string mLabel;
	};
}