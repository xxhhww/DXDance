#pragma once
#include "Game/ISystem.h"
#include <vector>
#include <memory>

namespace Game {

	class SystemManger {
	public:
		void Emplace(std::unique_ptr<ISystem>&& system);

		void Run();

	private:
		std::vector<std::unique_ptr<ISystem>> mSystems;
	};

}