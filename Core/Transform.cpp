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

	// 在Gui界面操作时，local数据可读写，world数据仅可读

	Math::Vector3		mLocalPosition;		// 相对于父节点的位置
	Math::Quaternion	mLocalRotation;		// 相对于父节点的旋转
	Math::Vector3		mLocalScale;		// 相对于父节点的缩放
	Math::Matrix4		mLocalMatrix;		// 相对于父节点的变换

	Math::Vector3		mWorldPosition;		// 相对于世界原点的位置
	Math::Quaternion	mWorldRotation;		// 相对于世界原点的旋转
	Math::Vector3		mWorldScale;		// 相对于世界原点的缩放
	Math::Matrix4		mWorldMatrix;		// 相对于世界原点的变换

	bool				mIsStatic{ false };	// 是否为静态物体

	void Transform::SerializeJson(Tool::JsonWriter& writer) const {
		using namespace Tool;

		writer.StartObject();

		SerializeHelper::SerializeString(writer, "Typename", std::string(typeid(Transform).name()));
		SerializeHelper::SerializeVector3(writer, "LocalPosition", mLocalPosition);
		SerializeHelper::SerializeQuaternion(writer, "LocalRotation", mLocalRotation);
		SerializeHelper::SerializeVector3(writer, "LocalScale", mLocalScale);
		SerializeHelper::SerializeMatrix(writer, "LocalMatrix", mLocalMatrix);

		SerializeHelper::SerializeVector3(writer, "WorldPosition", mWorldPosition);
		SerializeHelper::SerializeQuaternion(writer, "WorldRotation", mWorldRotation);
		SerializeHelper::SerializeVector3(writer, "WorldScale", mWorldScale);
		SerializeHelper::SerializeMatrix(writer, "WorldMatrix", mWorldMatrix);

		SerializeHelper::SerializeBool(writer, "Static", mIsStatic);

		writer.EndObject();
	}

	void Transform::DeserializeJson(const Tool::JsonReader& reader) {
		using namespace Tool;

		// Typename由Actor解析
		SerializeHelper::DeserializeVector3(reader, "LocalPosition", mLocalPosition);
		SerializeHelper::DeserializeQuaternion(reader, "LocalRotation", mLocalRotation);
		SerializeHelper::DeserializeVector3(reader, "LocalScale", mLocalScale);
		SerializeHelper::DeserializeMatrix(reader, "LocalMatrix", mLocalMatrix);

		SerializeHelper::DeserializeVector3(reader, "WorldPosition", mWorldPosition);
		SerializeHelper::DeserializeQuaternion(reader, "WorldRotation", mWorldRotation);
		SerializeHelper::DeserializeVector3(reader, "WorldScale", mWorldScale);
		SerializeHelper::DeserializeMatrix(reader, "WorldMatrix", mWorldMatrix);

		SerializeHelper::DeserializeBool(reader, "Static", mIsStatic);
	}

}