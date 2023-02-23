#pragma once
#include <vector>
#include <memory>
#include "IPlugin.h"

namespace UI {
	class Pluginable {
	public:
		void ExecuteAllPlugins();

		void DeleteAllPlugins();

		template<typename T, typename... Args>
		T& CreatePlugin(Args&&... args);

		template<typename T>
		T* GetPlugin();
	private:
		std::vector<std::unique_ptr<IPlugin>> mPlugins;
	};
}

#include "Pluginable.inl"