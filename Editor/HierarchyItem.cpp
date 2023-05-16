#include "HierarchyItem.h"

#include "Core/Actor.h"

#include "UI/imgui.h"

namespace App {

	HierarchyItem::HierarchyItem(Core::Actor* actor, bool root)
	: root(root)
	, mActor(actor) {
		mAutoExecutePlugins = false;
	}

	HierarchyItem::~HierarchyItem() {
		for (auto& item : mChilds) {
			if (item != nullptr) {
				delete item;
				item = nullptr;
			}
		}
		mChilds.clear();
	}

	void HierarchyItem::AttachParent(HierarchyItem* parent) {
		if (mParent != nullptr) {
			mParent->UnConsiderItem(this);
		}
		mParent = parent;
		mParent->ConsiderItem(this);
	}

	void HierarchyItem::ConsiderItem(HierarchyItem* targetItem) {
		// ȷ��Ψһ
		auto it = std::find_if(mChilds.begin(), mChilds.end(),
			[&](HierarchyItem* item) {
				return (item == targetItem) ? true : false;
			});
		if (it != mChilds.end()) {
			return;
		}

		mChilds.push_back(targetItem);
	}

	HierarchyItem* HierarchyItem::UnConsiderItem(HierarchyItem* targetItem) {
		// ȷ������
		auto it = std::find_if(mChilds.begin(), mChilds.end(),
			[&](HierarchyItem* item) {
				return (item == targetItem) ? true : false;
			});
		if (it == mChilds.end()) {
			return nullptr;
		}

		mChilds.erase(it);
		return targetItem;
	}

	void HierarchyItem::_Draw_Internal_Impl() {
		if (root) {
			// �����Ӷ���
			DoDestruction();
			// ��������Ŀ
			for (uint32_t i = 0; i < mChilds.size(); i++) {
				mChilds.at(i)->Draw();
			}
			return;
		}

		bool prevOpened = opened;

		ImGui::SetNextItemOpen(opened);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_None;
		flags |= ImGuiTreeNodeFlags_OpenOnArrow;

		opened = ImGui::TreeNodeEx((mActor->GetName() + mWidgetID).c_str(), flags);

		if (ImGui::IsItemClicked() && (ImGui::GetMousePos().x - ImGui::GetItemRectMin().x) > ImGui::GetTreeNodeToLabelSpacing()) {
			clickedEvent.Invoke();
			if (ImGui::IsMouseDoubleClicked(ImGuiMouseButton_Left)) {
				doubleClickedEvent.Invoke();
			}
		}

		// ����Ⱦ�Ӷ���֮ǰִ�����²���
		ExecuteAllPlugins();
		DoDestruction();

		if (opened) {
			if (!prevOpened) {
				openedEvent.Invoke();
			}

			// ��������Ŀ
			for (uint32_t i = 0; i < mChilds.size(); i++) {
				mChilds.at(i)->Draw();
			}

			ImGui::TreePop();
		}
		else {
			if (prevOpened) {
				closedEvent.Invoke();
			}
		}
	}

	void HierarchyItem::DoDestruction() {
		mChilds.erase(
			std::remove_if(mChilds.begin(), mChilds.end(),
				[](HierarchyItem*& item) {
					bool needErase = item->IsDestory();
					if (needErase) {
						delete item;
						item = nullptr;
					}
					return needErase;
				}),
			mChilds.end());
	}

}