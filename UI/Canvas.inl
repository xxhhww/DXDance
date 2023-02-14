#pragma once
#include "Canvas.h"
#include <assert.h>

namespace UI {
	template<typename T, typename ...Args>
	T& Canvas::CreatePanel(Args&&... args) {
		if constexpr (!std::is_base_of<IPanel, T>::value) {
			assert(false);
		}
		mPanels.emplace_back(new T(args...), CanvasMemoryMode::Internal);
		T& instance = *reinterpret_cast<T*>(mPanels.back().first);
		return instance;
	}
}