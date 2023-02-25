#include "InputText.h"
#include "imgui.h"

namespace UI {
	InputText::InputText(const std::string& label, const std::string& content, size_t maxSize)
	: IDataWidget<std::string>(content)
	, mLabel(label) 
	, mMaxSize(maxSize) {
		// data.resize(mMaxSize, '\0');
	}

	void InputText::_Draw_Internal_Impl() {
		data.resize(mMaxSize, '\0');
		bool enterPressed = ImGui::InputText((mLabel + mWidgetID).c_str(), &data[0], mMaxSize, ImGuiInputTextFlags_EnterReturnsTrue);
		data = data.c_str();
		if (enterPressed) {
			enterPressedEvent.Invoke(data);
		}
	}
}