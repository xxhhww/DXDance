#pragma once
#include "Game/ISystem.h"
#include <vector>
#include <memory>

namespace Game {

	class SystemManger {
	public:
		~SystemManger();

		void Create();

		void Emplace(std::unique_ptr<ISystem>&& system);

		void PrePhysicsUpdate();

		void PostPhysicsUpdate();

	private:
		std::vector<std::unique_ptr<ISystem>> mSystems;
	};

}