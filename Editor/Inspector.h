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

		void FocusActor(Core::Actor* actor);

		void UnFocusActor();

		inline auto* GetFocusActor() const { return mSelectedActor; }

	private:
		Core::Actor* mSelectedActor{ nullptr };
		UI::Group*   mActorComponentsGroup{ nullptr };
	};

}