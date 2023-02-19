#include "Pluginable.h"

namespace UI {
	void Pluginable::ExecuteAllPlugins() {
		for (auto& plugin : mPlugins) {
			plugin->Execute();
		}
	}

	void Pluginable::DeleteAllPlugins() {
		mPlugins.clear();
	}
}