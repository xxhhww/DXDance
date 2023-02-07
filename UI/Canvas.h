#pragma once
#include "IPanel.h"

namespace UI {
	class Canvas : public IDrawable {
	public:
		void AddPanel(IPanel* panel);
		void RemovePanel(IPanel* panel);
		void RemoveAllPanels();
		void SetDockspace(bool status);
		bool IsDockspace() const;

		void Draw() override;
	private:
		std::vector<IPanel*> mPanels;
		bool mDockSpace{ false };
	};
}