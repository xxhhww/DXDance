#include "Group.h"
#include "imgui.h"

namespace UI {
	void Group::_Draw_Internal_Impl() {
		ImGui::BeginGroup();
		DrawWidgets();
		ImGui::EndGroup();
	}

	GroupCollapsable::GroupCollapsable(const std::string& name) 
	: mName(name) {}

	void GroupCollapsable::_Draw_Internal_Impl() {
		bool prevOpenStatus = mOpenStatus;

		ImGuiTreeNodeFlags flags{};
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		mOpenStatus = ImGui::CollapsingHeader((mName + mWidgetID).c_str(), flags);
		if (ImGui::IsItemClicked() && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing()) {
			if (ImGui::IsMouseClicked(0)) {
				clickedEvent.Invoke();
			}
			else if (ImGui::IsMouseDoubleClicked(0)) {
				doubleClickedEvent.Invoke();
			}
		}

		if (mOpenStatus) {
			if (!prevOpenStatus) {
				openedEvent.Invoke();
			}
			DrawWidgets();
		}
		else {
			if (prevOpenStatus) {
				closedEvent.Invoke();
			}
		}
	}
}