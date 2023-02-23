#include "TreeNode.h"
#include "imgui.h"

namespace UI {
	TreeNode::TreeNode(const std::string& name, bool isLeaf)
	: mName(name) 
	, mIsLeaf(isLeaf) {
		mAutoExecutePlugins = false;
	}

	void TreeNode::_Draw_Internal_Impl() {
		bool prevOpenStatus = mOpenStatus;

		ImGuiTreeNodeFlags flags{};
		flags |= ImGuiTreeNodeFlags_OpenOnDoubleClick;
		if (mIsLeaf) flags |= ImGuiTreeNodeFlags_Leaf;

		mOpenStatus = ImGui::TreeNodeEx((mName + mWidgetID).c_str(), flags);
		if (ImGui::IsItemClicked() && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing()) {
			if(ImGui::IsMouseClicked(0)){
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