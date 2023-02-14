#include "MenuList.h"
#include "imgui.h"

namespace UI {
	MenuItem::MenuItem(const std::string& name, bool checkable, bool checkStatus)
	: mName(name) 
	, mCheckable(checkable) 
	, mCheckStatus(checkStatus) {}

	void MenuItem::_Draw_Internal_Impl() {
		bool prevCheckStatus = mCheckStatus;
		if (ImGui::MenuItem((mName + mWidgetID).c_str(), nullptr, mCheckable ? &mCheckStatus : nullptr, mEnable)) {
			clickedEvent.Invoke();
		}
		if (prevCheckStatus != mCheckStatus) {
			checkStatusChangedEvent.Invoke(mCheckStatus);
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
			ImGui::EndMenu();
		}
		else {
			mOpenStatus = false;
		}
	}
}