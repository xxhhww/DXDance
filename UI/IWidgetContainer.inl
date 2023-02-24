#pragma once
#include "IWidgetContainer.h"
#include <assert.h>

namespace UI {
	template <typename T, typename ...Args>
	T& IWidgetContainer::CreateWidget(Args&&... args) {
		static_assert(std::is_base_of<IWidget, T>::value, "T should derive from IWidget");
		mWidgets.emplace_back(new T(args...), IWidgetMangement::InternalMangement);
		T& instance = *reinterpret_cast<T*>(mWidgets.back().first);
		instance.SetParent(this);
		return instance;
	}

	template<typename T, typename ...Args>
	T& IWidgetContainer::CreateWidgetDelay(Args&&... args) {
		static_assert(std::is_base_of<IWidget, T>::value, "T should derive from IWidget");
		mDelayWidgets.emplace(new T(args...));
		T& instance = *reinterpret_cast<T*>(mDelayWidgets.back());
		instance.SetParent(this);
		return instance;
	}
}