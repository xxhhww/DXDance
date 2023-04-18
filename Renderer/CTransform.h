#pragma once
#include "ECS/IComponent.h"

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/Matrix.h"

namespace Renderer {

	class Transform : public ECS::IComponent {
	public:
		Math::Vector3		worldPosition;	// ���������ԭ���λ��
		Math::Quaternion	worldRotation;	// ���������ԭ�����ת
		Math::Vector3		worldScale;		// ���������ԭ�������
		Math::Matrix4		worldMatrix;	// ���������ԭ��ı任

	public:
		/*
		* ���л�ΪJson����
		*/
		void SerializeJson(Tool::JsonWriter& writer) const override;

		/*
		* �����л�Json����
		*/
		void DeserializeJson(const Tool::JsonReader& reader) override;
	};

}