#pragma once
#include "IDataWidget.h"

namespace UI {
	class ComboBox : public IDataWidget<int> {
	public:
		ComboBox(const std::string& label, int currentChoice = 0u);
	protected:
		void _Draw_Internal_Impl() override;
	public:
		std::unordered_map<int, std::string> choices;
	private:
		std::string mLabel;
	};
}