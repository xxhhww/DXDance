#pragma once
#include "Game/ISystem.h"

namespace Game {

	class StreamPhysicsSystem : public ISystem {
	public:
		virtual void Create() override;

		virtual void Destory() override;

		virtual void Run() override;
	};

}