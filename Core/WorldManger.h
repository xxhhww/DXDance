#pragma once
#include "Core/World.h"

namespace Core {

	class WorldManger {
	public:
		WorldManger();
		~WorldManger();

		void CreateEmptyWorld();

	private:
		World* mWorld{ nullptr };
	};

}