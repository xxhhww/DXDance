#include "Text.h"

namespace UI {
	Text::Text(const std::string& content)
	: IDataWidget<std::string>(content) {}

	void Text::_Draw_Internal_Impl() {
		ImGui::Text(data.c_str());
	}

}