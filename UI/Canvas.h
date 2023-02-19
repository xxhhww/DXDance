#pragma once
#include "IPanel.h"
#include <unordered_map>
#include <memory>

namespace UI {
	class Canvas : public IDrawable {
	public:
		void DeleteAllPanels();

		template<typename T, typename ...Args>
		T& CreatePanel(const std::string& name, Args&&... args);

		template<typename T>
		T& GetPanel(const std::string& name);

		void SetDockspace(bool status);
		bool IsDockspace() const;

		void Draw() override;
	private:
		std::unordered_map<std::string, std::unique_ptr<IPanel>> mPanels;
		// std::vector<std::pair<IPanel*, CanvasMemoryMode>> mPanels;
		bool mDockSpace{ false };
	};
}

#include "Canvas.inl"