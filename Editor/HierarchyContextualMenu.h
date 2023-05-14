#pragma once
#include "UI/ContextualMenu.h"

namespace Core {
	class Actor;
}

namespace App {

	class HierarchyContextualMenu : public UI::ContextualMenu {
	public:
		HierarchyContextualMenu(Core::Actor* actor, UI::ContextualMenuType menuType);
		~HierarchyContextualMenu();

	private:
		void BuildPopupContextItem();

	private:
		Core::Actor* mActor{ nullptr };
	};

}
