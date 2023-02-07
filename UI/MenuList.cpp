#include "MenuList.h"
#include "imgui.h"

namespace UI {
	MenuItem::MenuItem(const std::string& name, bool clickable)
	: mName(name) 
	, mClickable(clickable) {}

	void MenuItem::_Draw_Internal_Impl() {
		if (ImGui::MenuItem((mName + mWidgetID).c_str(), nullptr, mClickable ? &mIsClicked : nullptr, mEnable)) {
			if (mIsClicked) {
				clickedEvent.Invoke();
			}
		}
	}

	MenuList::MenuList(const std::string& name) 
	: mName(name) {}

	void MenuList::_Draw_Internal_Impl() {
		if (ImGui::BeginMenu((mName + mWidgetID).c_str(), mEnable)) {
			if (!mOpenStatus) {
				clickedEvent.Invoke();
				mOpenStatus = true;
			}
			DrawWidgets();
		}
		else {
			mOpenStatus = false;
		}
		ImGui::EndMenu();
	}
}