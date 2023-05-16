#pragma once
#include "UI/PanelWindow.h"

namespace Core {
	class Actor;
}

namespace UI {
	class Group;
}

namespace App {

	class Inspector : public UI::PanelWindow {
	public:
		Inspector(
			const std::string& title = "Inspector",
			bool opened = true,
			const UI::PanelWindowSettings& panelSetting = UI::PanelWindowSettings{}
		);

		~Inspector();

		void Focus(Core::Actor* actor);

		void UnFocus();

	private:
		Core::Actor* mSelectedActor{ nullptr };
		UI::Group*   mActorComponentsGroup{ nullptr };
	};

}