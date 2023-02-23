#pragma once
#include "Pluginable.h"

namespace UI {
	template<typename T, typename... Args>
	T& Pluginable::CreatePlugin(Args&&... args) {
		static_assert(std::is_base_of<IPlugin, T>::value, "T should derive from IPlugin");
		mPlugins.emplace_back(std::make_unique<T>(std::forward<Args>(args)...));
		return *reinterpret_cast<T*>(mPlugins.back().get());
	}

	template<typename T>
	T* Pluginable::GetPlugin() {
		static_assert(std::is_base_of<IPlugin, T>::value, "T should derive from IPlugin");
		for (auto it = mPlugins.begin(); it != mPlugins.end(); it++) {
			T* result = dynamic_cast<T*>((*it).get());
			if (result) {
				return result;
			}
		}
		return nullptr;
	}
}