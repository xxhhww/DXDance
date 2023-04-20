#pragma once
#include "ECS/IComponent.h"

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/Matrix.h"

namespace Renderer {

	class Transform : public ECS::IComponent {
	public:
		Math::Vector3	worldPosition;	// ���������ԭ���λ��
		Math::Vector3	worldRotation;	// ���������ԭ�����ת
		Math::Vector3	worldScale;		// ���������ԭ�������
		Math::Matrix4	worldMatrix;	// ���������ԭ��ı任

	public:
		Transform()
		: worldPosition(0.0f, 0.0f, 0.0f)
		, worldRotation(0.0f, 0.0f, 0.0f)
		, worldScale(1.0f, 1.0f, 1.0f) {}

	public:
		/*
		* ���л�ΪJson����
		*/
		void SerializeJson(Tool::JsonWriter& writer) const override {
			using namespace Tool;

			writer.StartObject();

			SerializeHelper::SerializeString(writer, "Typename", std::string(typeid(Transform).name()));

			SerializeHelper::SerializeVector3(writer, "WorldPosition", worldPosition);
			SerializeHelper::SerializeVector3(writer, "WorldRotation", worldRotation);
			SerializeHelper::SerializeVector3(writer, "WorldScale", worldScale);
			SerializeHelper::SerializeMatrix(writer, "WorldMatrix", worldMatrix);

			writer.EndObject();
		}

		/*
		* �����л�Json����
		*/
		void DeserializeJson(const Tool::JsonReader& reader) override {
			using namespace Tool;

			// Typename���ϲ����
			SerializeHelper::DeserializeVector3(reader, "WorldPosition", worldPosition);
			SerializeHelper::DeserializeVector3(reader, "WorldRotation", worldRotation);
			SerializeHelper::DeserializeVector3(reader, "WorldScale", worldScale);
			SerializeHelper::DeserializeMatrix(reader, "WorldMatrix", worldMatrix);
		}

	};

}