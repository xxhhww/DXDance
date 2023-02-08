#pragma once
#include "IWidgetContainer.h"
#include <assert.h>

namespace UI {
	template <typename T, typename ...Args>
	T& IWidgetContainer::CreateWidget(Args&&... args) {
		if constexpr (!std::is_base_of<IWidget, T>::value) {
			assert(false);
		}
		mWidgets.emplace_back(new T(args...), IWidgetMangement::InternalMangement);
		T& instance = *reinterpret_cast<T*>(mWidgets.back().first);
		instance.SetParent(this);
		return instance;
	}
}