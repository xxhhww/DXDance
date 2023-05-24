#pragma once
#include <cstdint>
#include "Math/Vector.h"

namespace Core {
	class Actor;
}

namespace App {

	enum class EAxisDirection : uint8_t {
		X = 0,
		Y = 1,
		Z = 2,
		COUNT = 3
	};

	enum class EAxisOperation : uint8_t {
		TRANSLATE = 0,
		ROTATE    = 1,
		SCALE     = 2,
		COUNT     = 3
	};

	class AxisBehavior {
	public:
		void StartPicking(Core::Actor* targetActor, const Math::Vector3& cameraPosition, EAxisDirection axisDirection, EAxisOperation axisOperation);

		void StopPicking();
		
		inline const auto IsPicking()    const { return mTargetActor != nullptr; }
		inline const auto GetDirection() const { return mAxisDirection; }
		inline const auto GetOperation() const { return mAxisOperation; }

	private:
		Core::Actor* mTargetActor{ nullptr };
		float mDistanceToActor; // �������Actor�ľ��룬����ԽԶ�ƶ�Խ�죬��֮��Խ��
		EAxisDirection mAxisDirection;
		EAxisOperation mAxisOperation;
	};

}