#pragma once
#include "UI/PanelWindow.h"

#include <unordered_map>

namespace UI {
	class Child;
}

namespace Core {
	class Actor;
}

namespace App {

	class HierarchyItem;

	class Hierarchy : public UI::PanelWindow {
	public:
		Hierarchy(
			const std::string& title = "Hierarchy",
			bool opened = true,
			const UI::PanelWindowSettings& panelSetting = UI::PanelWindowSettings{}
		);

		~Hierarchy();

		void CreateActorCallback(Core::Actor* actor);

		void DestoryActorCallback(Core::Actor* actor);

		void AttachActorCallback(Core::Actor* childActor, Core::Actor* parentActor);

	private:

	private:
		UI::Child* mCanvas{ nullptr };
		HierarchyItem* mRootItem{ nullptr };
		std::unordered_map<Core::Actor*, HierarchyItem*> mHelperLinks;
	};

}