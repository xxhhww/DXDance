#include "HierarchyContextualMenu.h"

#include "Core/SceneManger.h"
#include "Core/ServiceLocator.h"

#include "UI/MenuList.h"
#include "UI/InputText.h"

#include "Tools/Assert.h"

namespace App {

	HierarchyContextualMenu::HierarchyContextualMenu(Core::Actor* actor, UI::ContextualMenuType menuType)
	: UI::ContextualMenu(menuType) 
	, mActor(actor) {
		BuildPopupContextItem();
	}

	HierarchyContextualMenu::~HierarchyContextualMenu() {
	}

	void HierarchyContextualMenu::BuildPopupContextItem() {

		// Create
		auto& createMenu = CreateWidget<UI::MenuList>("Create");
		auto& createEmptyItem = createMenu.CreateWidget<UI::MenuItem>("Empty");
		createEmptyItem.clickedEvent += [this]() {
			auto* currScene = CORESERVICE(Core::SceneManger).GetCurrentScene();
			ASSERT_FORMAT(currScene != nullptr, "Current Scene Must Not Be Nullptr");
			auto* actor = currScene->CreateActor("Empty");
			if (mActor != nullptr) {
				actor->AttachParent(mActor);
			}
		};

		auto& createSphereItem = createMenu.CreateWidget<UI::MenuItem>("Sphere");

		if (mActor != nullptr) {
			// Delete
			auto& deleteMenuItem = CreateWidget<UI::MenuItem>("Delete");
			deleteMenuItem.clickedEvent += [this]() {
				auto* currScene = CORESERVICE(Core::SceneManger).GetCurrentScene();
				ASSERT_FORMAT(currScene != nullptr, "Current Scene Must Not Be Nullptr");
				currScene->DeleteActor(mActor);
			};

			// Rename
			auto& renameMenu = CreateWidget<UI::MenuList>("Rename");
			auto& renameInputTextItem = renameMenu.CreateWidget<UI::InputText>("##hidelabel", "");
		} 
	}

}