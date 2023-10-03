#pragma once
#include "Game/ISystem.h"

namespace Game {

	class StreamPhysicsSystem : public ISystem {
	public:
		StreamPhysicsSystem();

		virtual void Run() override;
	};

}