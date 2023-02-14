#pragma once
#include "IPanel.h"

namespace UI {
	enum class CanvasMemoryMode {
		Internal, Extra
	};

	class Canvas : public IDrawable {
	public:
		void RegisterPanel(IPanel* panel);
		void UnregisterPanel(IPanel* panel);

		void DeletePanel(IPanel* panel);
		void DeleteAllPanels();

		template<typename T, typename ...Args>
		T& CreatePanel(Args&&... args);

		void SetDockspace(bool status);
		bool IsDockspace() const;

		void Draw() override;
	private:
		std::vector<std::pair<IPanel*, CanvasMemoryMode>> mPanels;
		bool mDockSpace{ false };
	};
}

#include "Canvas.inl"