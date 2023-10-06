#pragma once
#include "Game/ISystem.h"

namespace Game {

	class TankSystem : public ISystem {
	public:
		virtual void Create() override;

		virtual void Destory() override;

		virtual void PrePhysicsUpdate() override;

		virtual void PostPhysicsUpdate() override;

	private:
	};

}