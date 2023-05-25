#pragma once
#include "ECS/CTransform.h"

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
		
		void SetCurrentMousePos(const Math::Vector2& mousePos);

		void ApplyOperation(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize);

		inline const auto IsPicking()    const { return mTargetActor != nullptr; }
		inline const auto GetDirection() const { return mAxisDirection; }
		inline const auto GetOperation() const { return mAxisOperation; }

	private:
		void ApplyTransition(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize);
		void ApplyRotation(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize);
		void ApplyScale(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize);

		Math::Vector2 GetScreenDirection(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize) const;

		Math::Vector3 GetRealDirection() const;
		Math::Vector3 GetFakeDirection() const;

	private:
		Core::Actor*   mTargetActor{ nullptr };
		ECS::Transform mOriginalTransform;

		float mDistanceToActor; // 摄像机与Actor的距离，距离越远移动越快，反之则越慢
		EAxisDirection mAxisDirection;
		EAxisOperation mAxisOperation;

		bool mFirstMouse{ true };
		Math::Vector2 mOriginMouse;
		Math::Vector2 mCurrentMouse;
	};

}