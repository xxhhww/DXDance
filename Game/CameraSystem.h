#pragma once
#include "Game/ISystem.h"

namespace Game {

	class CameraSystem : public ISystem {
	public:
		virtual void Run() override;
	};

}