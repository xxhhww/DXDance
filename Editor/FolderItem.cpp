#include "FolderItem.h"
#include "UI/imgui.h"

namespace App {
	FolderItem::FolderItem(const std::string& name, const std::string& path)
	: BrowserItem(name, path) {
		mAutoExecutePlugins = false;
	}

	void FolderItem::DoDestruction() {
		for (auto it = mBrowserItems.begin(); it != mBrowserItems.end();) {
			if (it->second->IsDestory()) {
				it = mBrowserItems.erase(it);
			}
			else {
				it++;
			}
		}
	}

	void FolderItem::_Draw_Internal_Impl() {
		bool prevOpened = opened;

		ImGui::SetNextItemOpen(opened);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;
		
		opened = ImGui::TreeNodeEx((name + mWidgetID).c_str(), flags);

		if (ImGui::IsItemClicked() && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing()) {
			clickedEvent.Invoke();
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				doubleClickedEvent.Invoke();
			}
		}

		// 执行销毁操作
		DoDestruction();

		if (opened) {
			if (!prevOpened) {
				openedEvent.Invoke();
			}

			ExecuteAllPlugins(); // Manually execute plugins to make plugins considering the TreeNode and no childs

			// 绘制子项目
			for (auto& pair : mBrowserItems) {
				pair.second->Draw();
			}

			ImGui::TreePop();
		}
		else {
			if (prevOpened) {
				closedEvent.Invoke();
			}

			ExecuteAllPlugins();
		}
	}
}