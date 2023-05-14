#include "Child.h"
#include "imgui.h"

namespace UI {
	Child::Child(const std::string& name, float widthScale, bool border) 
	: mName(name)
	, mWidthScale(widthScale)
	, useBorder(border) {
		mAutoExecutePlugins = false;
	}

	void Child::_Draw_Internal_Impl() {
		ImGuiWindowFlags flags{};

        ImGui::BeginChild((mName + mWidgetID).c_str(), ImVec2(ImGui::GetWindowContentRegionWidth() * mWidthScale, 0), useBorder, flags);

		DrawWidgets();
		ExecuteAllPlugins();

        ImGui::EndChild();
	}
}