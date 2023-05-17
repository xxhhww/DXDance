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
		FocusActor(currScene->FindActorByName("MainCamera"));
	}

	Inspector::~Inspector() {}

	void Inspector::FocusActor(Core::Actor* actor) {
		if (actor == mSelectedActor) {
			return;
		}

		UnFocusActor();
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

	void Inspector::UnFocusActor() {
		if (mSelectedActor == nullptr) {
			return;
		}
		// 注意：有内存泄露的风险！！！后续再优化
		mActorComponentsGroup->DeleteAllWidgets();
		mSelectedActor = nullptr;
	}

}