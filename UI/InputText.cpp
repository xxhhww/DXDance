#include "InputText.h"
#include "imgui.h"

namespace UI {
	InputText::InputText(const std::string& label, const std::string& content, size_t maxSize)
	: IDataWidget<std::string>(content)
	, mLabel(label) 
	, mMaxSize(maxSize) {
		mData.resize(mMaxSize, '\0');
	}

	void InputText::_Draw_Internal_Impl() {
		bool editCompleted = ImGui::InputText((mLabel + mWidgetID).c_str(), &mData[0], mMaxSize, ImGuiInputTextFlags_EnterReturnsTrue);
		if (editCompleted) {
			editCompletedEvent.Invoke(mData);
		}
	}
}