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

	// ��Gui�������ʱ��local���ݿɶ�д��world���ݽ��ɶ�

	Math::Vector3		mLocalPosition;		// ����ڸ��ڵ��λ��
	Math::Quaternion	mLocalRotation;		// ����ڸ��ڵ����ת
	Math::Vector3		mLocalScale;		// ����ڸ��ڵ������
	Math::Matrix4		mLocalMatrix;		// ����ڸ��ڵ�ı任

	Math::Vector3		mWorldPosition;		// ���������ԭ���λ��
	Math::Quaternion	mWorldRotation;		// ���������ԭ�����ת
	Math::Vector3		mWorldScale;		// ���������ԭ�������
	Math::Matrix4		mWorldMatrix;		// ���������ԭ��ı任

	bool				mIsStatic{ false };	// �Ƿ�Ϊ��̬����

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

		// Typename��Actor����
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