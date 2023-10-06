#pragma once
#include "ECS/IComponent.h"
#include "Jolt/Jolt.h"
#include "Jolt/Physics/Vehicle/VehicleConstraint.h"
#include "Jolt/Physics/Constraints/HingeConstraint.h"

namespace Game {

	class CTank : public ECS::IComponent {
	public:
		// 坦克物理属性
		float mass{ 4000.0f };

		// 坦克行为控制参数
		float prevForward{ 1.0f };
		float forward{ 0.0f };		
		float leftRatio{ 1.0f };
		float rightRatio{ 1.0f };
		float brake{ 0.0f };

		JPH::Ref<JPH::VehicleConstraint> vehicleConstraint;
		JPH::BodyID tankBodyID;
	};

}