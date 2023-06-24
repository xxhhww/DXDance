#include "AxisBehavior.h"

#include "Core/Actor.h"

#include "ECS/CTransform.h"

namespace App {

	void AxisBehavior::StartPicking(Core::Actor* targetActor, const Math::Vector3& cameraPosition, EAxisDirection axisDirection, EAxisOperation axisOperation) {
		if (mTargetActor != nullptr) {
			return;
		}

		mTargetActor = targetActor;
		mAxisDirection = axisDirection;
		mAxisOperation = axisOperation;

		mOriginalTransform = mTargetActor->GetComponent<ECS::Transform>();
		mDistanceToActor = (cameraPosition - mOriginalTransform.worldPosition).Length();
	}

	void AxisBehavior::StopPicking() {
		mTargetActor = nullptr;
		mDistanceToActor = 0.0f;
		mFirstMouse = true;
	}

	void AxisBehavior::SetCurrentMousePos(const Math::Vector2& mousePos) {
		if (mFirstMouse) {
			mCurrentMouse = mOriginMouse = mousePos;
			mFirstMouse = false;
		}
		else {
			mCurrentMouse = mousePos;
		}
	}

	void AxisBehavior::ApplyOperation(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize) {
		switch (mAxisOperation) {
		case EAxisOperation::TRANSLATE:
			ApplyTransition(viewMatrix, projectionMatrix, viewSize);
			break;

		case EAxisOperation::ROTATE:
			ApplyRotation(viewMatrix, projectionMatrix, viewSize);
			break;

		case EAxisOperation::SCALE:
			ApplyScale(viewMatrix, projectionMatrix, viewSize);
			break;
		}
	}

	void AxisBehavior::ApplyTransition(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize) {
		auto unitsPerPixel = 0.001f * mDistanceToActor;
		auto originPosition = mOriginalTransform.worldPosition;

		auto screenDirection = GetScreenDirection(viewMatrix, projectionMatrix, viewSize);

		auto totalDisplacement = mCurrentMouse - mOriginMouse;
		auto translationCoefficient = totalDisplacement.Dot(screenDirection) * unitsPerPixel;

		auto& targetTransform = mTargetActor->GetComponent<ECS::Transform>();
		targetTransform.worldPosition = (originPosition + GetRealDirection() * translationCoefficient);
	}
	
	void AxisBehavior::ApplyRotation(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize) {
		auto unitsPerPixel = 0.2f;
		auto originRotation = mOriginalTransform.worldRotation;

		auto screenDirection = GetScreenDirection(viewMatrix, projectionMatrix, viewSize);
		screenDirection = Math::Vector2(-screenDirection.y, screenDirection.x);

		auto totalDisplacement = mCurrentMouse - mOriginMouse;
		auto rotationCoefficient = totalDisplacement.Dot(screenDirection) * unitsPerPixel;

		auto rotationToApply = Math::Quaternion(Math::Vector3(GetFakeDirection() * rotationCoefficient));
		
		auto& targetTransform = mTargetActor->GetComponent<ECS::Transform>();
		targetTransform.worldRotation = (Math::Quaternion{ originRotation } * rotationToApply).VecYxz();
	}

	void AxisBehavior::ApplyScale(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize) {
		return;
	}

	Math::Vector2 AxisBehavior::GetScreenDirection(const Math::Matrix4& viewMatrix, const Math::Matrix4& projectionMatrix, const Math::Vector2& viewSize) const {
		auto start = mOriginalTransform.worldPosition;
		auto end = mOriginalTransform.worldPosition + GetRealDirection() * 0.01f;

		auto start2D = Math::Vector2();
		{
			auto clipSpacePos = (Math::Vector4{ start.x, start.y, start.z, 1.0f } *viewMatrix) * projectionMatrix;
			auto ndcSpacePos = Math::Vector3{ clipSpacePos.x, clipSpacePos.y, clipSpacePos.z } / clipSpacePos.w;
			auto windowSpacePos = ((Math::Vector2{ ndcSpacePos.x, ndcSpacePos.y } + 1.0f) / 2.0f);
			windowSpacePos.x *= viewSize.x;
			windowSpacePos.y *= viewSize.y;
			start2D = windowSpacePos;
		}

		auto end2D = Math::Vector2();
		{
			auto clipSpacePos = (Math::Vector4{ end.x, end.y, end.z, 1.0f } *viewMatrix) * projectionMatrix;
			auto ndcSpacePos = Math::Vector3{ clipSpacePos.x, clipSpacePos.y, clipSpacePos.z } / clipSpacePos.w;
			auto windowSpacePos = ((Math::Vector2{ ndcSpacePos.x, ndcSpacePos.y } + 1.0f) / 2.0f);
			windowSpacePos.x *= viewSize.x;
			windowSpacePos.y *= viewSize.y;
			end2D = windowSpacePos;
		}

		auto result = end2D - start2D;

		result.y *= -1; // Screen coordinates are reversed, so we inverse the Y

		return result.Normalize();
	}

	Math::Vector3 AxisBehavior::GetRealDirection() const {
		auto result = Math::Vector3();

		switch (mAxisDirection) {
		case EAxisDirection::X:
			result = Math::Vector3{ 1.0f, 0.0f, 0.0f }.TransformAsVector(mOriginalTransform.worldRotation.RotationMatrix());
			break;
		case EAxisDirection::Y:
			result = Math::Vector3{ 0.0f, 1.0f, 0.0f }.TransformAsVector(mOriginalTransform.worldRotation.RotationMatrix());
			break;
		case EAxisDirection::Z:
			result = Math::Vector3{ 0.0f, 0.0f, 1.0f }.TransformAsVector(mOriginalTransform.worldRotation.RotationMatrix());
			break;
		}

		return result;
	}

	Math::Vector3 AxisBehavior::GetFakeDirection() const {
		auto result = Math::Vector3();

		switch (mAxisDirection) {
		case EAxisDirection::X:
			result = Math::Vector3{ 1.0f, 0.0f, 0.0f };
			break;
		case EAxisDirection::Y:
			result = Math::Vector3{ 0.0f, 1.0f, 0.0f };
			break;
		case EAxisDirection::Z:
			result = Math::Vector3{ 0.0f, 0.0f, 1.0f };
			break;
		}

		return result;
	}
}