#pragma once
#include "Canvas.h"
#include <assert.h>
#include "PanelWindow.h"

namespace UI {
	template<typename T, typename ...Args>
	T& Canvas::CreatePanel(const std::string& name, Args&&... args) {
		if constexpr (std::is_base_of<PanelWindow, T>::value) {
			mPanels.emplace(name, std::make_unique<T>(name, std::forward<Args>(args)...));
		}
		else {
			mPanels.emplace(name, std::make_unique<T>(std::forward<Args>(args)...));
		}
		T& instance = *static_cast<T*>(mPanels.at(name).get());
		return instance;
	}

	template<typename T>
	T& Canvas::GetPanel(const std::string& name) {
		return *static_cast<T*>(mPanels.at(name).get());
	}
}