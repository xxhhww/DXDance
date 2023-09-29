#pragma once
#include "Game/Context.h"
#include "Game/Game.h"
#include "GHL/DebugLayer.h"

#include <string>

namespace Game {
	class Application {
	public:
		Application(const std::string& name, HINSTANCE hInstance, int nCmdShow);

		~Application() = default;

		int Run();
	private:
		Context mContext;
		Game mGame;
	};
}
