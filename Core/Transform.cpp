#include "Transform.h"

namespace Core {
	void Transform::SetLocalPosition(const Math::Vector3& position) {
		mLocalPosition = position;
	}

	void Transform::SetLocalRotation(const Math::Quaternion& rotation) {
		mLocalRotation = rotation;
	}

	void Transform::SetLocalScale(const Math::Vector3& scale) {
		mLocalScale = scale;
	}

	void Transform::SetStatic(bool isStatic) {
		mIsStatic = isStatic;
	}

	void Transform::SerializeJson(rapidjson::Document& doc) const {

	}

	void Transform::DeserializeJson(const rapidjson::Document& doc) {

	}
}