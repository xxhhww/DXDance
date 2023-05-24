#include "AxisBehavior.h"

#include "Core/Actor.h"

#include "ECS/CTransform.h"

namespace App {

	void AxisBehavior::StartPicking(Core::Actor* targetActor, const Math::Vector3& cameraPosition, EAxisDirection axisDirection, EAxisOperation axisOperation) {
		mTargetActor = targetActor;
		mAxisDirection = axisDirection;
		mAxisOperation = axisOperation;

		auto& transform = mTargetActor->GetComponent<ECS::Transform>();
		mDistanceToActor = (cameraPosition - transform.worldPosition).Length();
	}

	void AxisBehavior::StopPicking() {
	}

}