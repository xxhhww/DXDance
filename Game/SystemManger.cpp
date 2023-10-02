#include "Game/SystemManger.h"

namespace Game {

	void SystemManger::Emplace(std::unique_ptr<ISystem>&& system) {
		mSystems.emplace_back(std::move(system));
	}

	void SystemManger::Run() {
		for (auto& system : mSystems) {
			system->Run();
		}
	}

}