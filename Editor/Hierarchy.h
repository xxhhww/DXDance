#pragma once
#include "UI/PanelWindow.h"

namespace UI {

}

namespace App {

	class Hierarchy : public UI::PanelWindow {
	public:
		Hierarchy(
			const std::string& title = "Hierarchy",
			bool opened = true,
			const UI::PanelWindowSettings& panelSetting = UI::PanelWindowSettings{}
		);

		~Hierarchy();

	private:
	};

}