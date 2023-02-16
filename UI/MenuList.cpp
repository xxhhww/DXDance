#include "MenuList.h"
#include "imgui.h"

namespace UI {
	MenuItem::MenuItem(const std::string& name, bool checkable, bool checkStatus)
	: mName(name) 
	, mCheckable(checkable) 
	, checkStatus(checkStatus) {}

	void MenuItem::_Draw_Internal_Impl() {
		bool prevCheckStatus = checkStatus;
		if (ImGui::MenuItem((mName + mWidgetID).c_str(), nullptr, mCheckable ? &checkStatus : nullptr, mEnable)) {
			clickedEvent.Invoke();
		}
		if (prevCheckStatus != checkStatus) {
			checkStatusChangedEvent.Invoke(checkStatus);
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

	void MenuBar::_Draw_Internal_Impl() {
		if (ImGui::BeginMenuBar()) {
			DrawWidgets();
			ImGui::EndMenuBar();
		}
	}
}