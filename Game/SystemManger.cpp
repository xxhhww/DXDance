#include "Game/SystemManger.h"

namespace Game {

	SystemManger::~SystemManger() {
		for (auto& system : mSystems) {
			system->Destory();
		}
	}

	void SystemManger::Create() {
		for (auto& system : mSystems) {
			system->Create();
		}
	}

	void SystemManger::Emplace(std::unique_ptr<ISystem>&& system) {
		mSystems.emplace_back(std::move(system));
	}

	void SystemManger::PrePhysicsUpdate() {
		for (auto& system : mSystems) {
			if (system->IsEnable()) {
				system->PrePhysicsUpdate();
			}
		}
	}

	void SystemManger::PostPhysicsUpdate() {
		for (auto& system : mSystems) {
			if (system->IsEnable()) {
				system->PostPhysicsUpdate();
			}
		}
	}

}