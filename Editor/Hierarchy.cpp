#include "Editor/Hierarchy.h"
#include "Editor/HierarchyItem.h"
#include "Editor/HierarchyContextualMenu.h"
#include "Editor/Inspector.h"

#include "Core/Actor.h"
#include "Core/ServiceLocator.h"

#include "UI/TreeNode.h"
#include "UI/Child.h"
#include "UI/UIManger.h"

#include "Tools/Assert.h"

namespace App {
	Hierarchy::Hierarchy(
		const std::string& title,
		bool opened,
		const UI::PanelWindowSettings& panelSetting
	) 
	: PanelWindow(title, opened, panelSetting) {

		mCanvas = &CreateWidget<UI::Child>("Scene");
		mCanvas->CreatePlugin<HierarchyContextualMenu>(nullptr, UI::ContextualMenuType::Window);

		mRootItem = &mCanvas->CreateWidget<HierarchyItem>(nullptr, true);

		// 注册Actor创建与销毁的回调函数
		Core::Actor::ActorCreatedEvent   += std::bind(&Hierarchy::CreateActorCallback, this, std::placeholders::_1);
		Core::Actor::ActorDestoryedEvent += std::bind(&Hierarchy::DestoryActorCallback, this, std::placeholders::_1);
		Core::Actor::ActorAttachEvent    += std::bind(&Hierarchy::AttachActorCallback, this, std::placeholders::_1, std::placeholders::_2);
	}

	Hierarchy::~Hierarchy() {
		if (mRootItem != nullptr) {
			delete mRootItem;
			mRootItem = nullptr;
		}
	}

	void Hierarchy::CreateActorCallback(Core::Actor* actor) {
		HierarchyItem* item = mRootItem->CreateHierarchyItem<HierarchyItem>(actor);
		item->CreatePlugin<HierarchyContextualMenu>(actor, UI::ContextualMenuType::Item);
		item->clickedEvent += [actor]() {
			auto* canvas = CORESERVICE(UI::UIManger).GetCanvas();
			auto& inspectorPanel = canvas->GetPanel<Inspector>("Inspector");
			inspectorPanel.FocusActor(actor);
		};

		mHelperLinks.emplace(std::make_pair(actor, item));
	}

	void Hierarchy::DestoryActorCallback(Core::Actor* actor) {
		auto it = mHelperLinks.find(actor);
		if (it == mHelperLinks.end()) {
			return;
		}
		HierarchyItem* item = it->second;
		item->Destory();

		auto* canvas = CORESERVICE(UI::UIManger).GetCanvas();
		auto& inspectorPanel = canvas->GetPanel<Inspector>("Inspector");
		auto* focusActor = inspectorPanel.GetFocusActor();
		if (focusActor == actor) {
			inspectorPanel.UnFocusActor();
		}

		mHelperLinks.erase(it);
	}

	void Hierarchy::AttachActorCallback(Core::Actor* childActor, Core::Actor* parentActor) {
		HierarchyItem* childItem = mHelperLinks.at(childActor);
		HierarchyItem* parentItem = mHelperLinks.at(parentActor);

		ASSERT_FORMAT(childItem != nullptr && parentItem != nullptr, "Item Should Not Be Nullptr");

		childItem->AttachParent(parentItem);
	}

}