#pragma once
#include "ECS/IComponent.h"

namespace Game {

	class CTankWheel : public ECS::IComponent {
	public:
		int32_t parentTankEntity{ -1 };
		uint32_t tankWheelIndex{ 0u };	// ��������������������ͬһ��������

	private:
	};

}