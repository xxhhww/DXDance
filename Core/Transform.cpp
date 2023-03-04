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

	void Transform::SerializeBinary(Tool::OutputMemoryStream& blob) const {
		blob.Write(mLocalPosition);
		blob.Write(mLocalRotation);
		blob.Write(mLocalScale);
		blob.Write(mLocalMatrix);

		blob.Write(mWorldPosition);
		blob.Write(mWorldRotation);
		blob.Write(mWorldScale);
		blob.Write(mWorldMatrix);

		blob.Write(mIsStatic);
	}

	void Transform::DeserializeBinary(Tool::InputMemoryStream& blob) {
		blob.Read(mLocalPosition);
		blob.Read(mLocalRotation);
		blob.Read(mLocalScale);
		blob.Read(mLocalMatrix);

		blob.Read(mWorldPosition);
		blob.Read(mWorldRotation);
		blob.Read(mWorldScale);
		blob.Read(mWorldMatrix);

		blob.Read(mIsStatic);
	}

}