#include "TreeNode.h"
#include "imgui.h"

namespace UI {
	TreeNode::TreeNode(const std::string& name, bool isLeaf)
	: name(name) 
	, isLeaf(isLeaf) {
		mAutoExecutePlugins = false;
	}

	void TreeNode::_Draw_Internal_Impl() {
		bool prevOpenStatus = opened;

		ImGuiTreeNodeFlags flags{};
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		if (isLeaf) flags |= ImGuiTreeNodeFlags_Leaf;

		opened = ImGui::TreeNodeEx((name + mWidgetID).c_str(), flags);
		if (ImGui::IsItemClicked() && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing()) {
			if(ImGui::IsMouseClicked(ImGuiMouseButton_Left)){
				clickedEvent.Invoke();
			}
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				doubleClickedEvent.Invoke();
			}
		}

		if (opened) {
			if (!prevOpenStatus) {
				openedEvent.Invoke();
			}
			ExecuteAllPlugins(); // Manually execute plugins to make plugins considering the TreeNode and no childs
			DrawWidgets();
			ImGui::TreePop();
		}
		else {
			if (prevOpenStatus) {
				closedEvent.Invoke();
			}
			ExecuteAllPlugins(); // Manually execute plugins to make plugins considering the TreeNode and no childs
		}
	}
}