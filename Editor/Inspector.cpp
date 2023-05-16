#include "Inspector.h"

#include "Core/ServiceLocator.h"
#include "Core/SceneManger.h"

#include "UI/Group.h"
#include "UI/Text.h"
#include "UI/InputText.h"
#include "UI/DragFloat.h"
#include "UI/DragFloat2.h"
#include "UI/DragFloat3.h"
#include "UI/DragFloat4.h"

#include "Tools/Assert.h"

namespace App {

	Inspector::Inspector(
		const std::string& title,
		bool opened,
		const UI::PanelWindowSettings& panelSetting
	)
	: UI::PanelWindow(title, opened, panelSetting) {

		mActorComponentsGroup = &CreateWidget<UI::Group>();

		// 初始化时，将SelectedActor设置为场景中的MainCamera
		auto* currScene = CORESERVICE(Core::SceneManger).GetCurrentScene();
		ASSERT_FORMAT(currScene != nullptr, "Current Scene Should Not Be Nullptr");
		Focus(currScene->FindActorByName("MainCamera"));
	}

	Inspector::~Inspector() {

	}

	void Inspector::Focus(Core::Actor* actor) {
		UnFocus();
		mSelectedActor = actor;

		// actor id
		mActorComponentsGroup->CreateWidget<UI::Text>(std::to_string(mSelectedActor->GetID()));
		// name
		auto& inputTextItem = mActorComponentsGroup->CreateWidget<UI::InputText>("name", mSelectedActor->GetName());
		inputTextItem.valueChangedEvent += [this](const std::string& newName) {
			mSelectedActor->SetName(newName);
		};
		// for each component
		mSelectedActor->ForeachComp([this](ECS::IComponent* component) {
			component->OnInspector(mActorComponentsGroup);
		});
	}

	void Inspector::UnFocus() {
		if (mSelectedActor == nullptr) {
			return;
		}
	}

}