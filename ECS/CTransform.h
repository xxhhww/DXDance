#pragma once
#include "ECS/IComponent.h"

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/Matrix.h"

#include "Jolt/Jolt.h"

namespace ECS {

	class Transform : public ECS::IComponent {
	public:
		Math::Vector3		worldPosition;	// ���������ԭ���λ��
		Math::Quaternion	worldRotation;	// ���������ԭ�����ת(������)
		Math::Vector3		worldScaling;	// ���������ԭ�������

		Math::Matrix4	prevworldMatrix;	// ���������ԭ��ı任
		Math::Matrix4	currWorldMatrix;

	public:
		Transform()
		: worldPosition(0.0f, 0.0f, 0.0f)
		, worldRotation(Math::Quaternion{})
		, worldScaling(1.0f, 1.0f, 1.0f) {}

		inline Math::Vector3 GetDirection() const {
			// XMMatrixRotationRollPitchYaw����ת�����ǴӸ�������������򿴹�ȥʱ��˳ʱ�뷽��
			// ���ʹ��{ 0.0f, 0.0f, -1.0f }������{ 0.0f, 0.0f, 1.0f }
			return Math::Vector3{ 0.0f, 0.0f, -1.0f }.TransformAsVector(worldRotation.RotationMatrix());
		}

		inline Math::Matrix4 GetWorldMatrix() {
			return Math::Matrix4(worldPosition, worldRotation, worldScaling);
		}


		/*
		* ���������ǻ�����������ϵ�ģ������Ҫ����������ת��ת������������ϵ
		*/
		inline JPH::Vec3 GetPhysicalSpaceWorldPosition() const {
			return JPH::Vec3{ worldPosition.x, worldPosition.y, -worldPosition.z };
		};

		inline JPH::Quat GetPhysicalSpaceWorldRotation() const {
			return JPH::Quat{ worldRotation.x, worldRotation.y, worldRotation.z, worldRotation.w };
		};

		inline void SetPhysicalSpaceWorldPosition(const JPH::Vec3& position) {
			worldPosition = Math::Vector3{ position.GetX(), position.GetY(), -position.GetZ() };
		};

		inline void SetPhysicalSpaceWorldRotation(const JPH::Quat& rotation) {
			worldRotation = Math::Quaternion{ -rotation.GetX(), rotation.GetY(), -rotation.GetZ(), rotation.GetW() };
		};

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {
			/*
			using namespace Tool;

			writer.StartObject();

			SerializeHelper::SerializeString(writer, "Typename", std::string(typeid(Transform).name()));

			SerializeHelper::SerializeVector3(writer, "WorldPosition", worldPosition);
			SerializeHelper::SerializeVector3(writer, "WorldRotation", worldRotation);
			SerializeHelper::SerializeVector3(writer, "WorldScaling", worldScaling);
			SerializeHelper::SerializeMatrix(writer, "WorldMatrix", worldMatrix);

			writer.EndObject();
			*/
		}

		void DeserializeJson(const Tool::JsonReader& reader) override {
			/*
			using namespace Tool;

			// Typename���ϲ����
			SerializeHelper::DeserializeVector3(reader, "WorldPosition", worldPosition);
			SerializeHelper::DeserializeVector3(reader, "WorldRotation", worldRotation);
			SerializeHelper::DeserializeVector3(reader, "WorldScaling", worldScaling);
			SerializeHelper::DeserializeMatrix(reader, "WorldMatrix", worldMatrix);
			*/
		}

		void OnInspector(UI::IWidgetContainer* container) override {
			/*
			auto* group = &container->CreateWidget<UI::GroupCollapsable>("Transform");
			
			auto& posItem = group->CreateWidget<UI::DragFloat3>("Position", worldPosition);
			posItem.dataGatherer = [this]() -> Math::Vector3 {
				return worldPosition;
			};
			posItem.dataProvider = [this](Math::Vector3 newValue) {
				worldPosition = newValue;
			};

			auto& rotItem = group->CreateWidget<UI::DragFloat3>("Rotation", worldRotation);
			rotItem.dataGatherer = [this]() -> Math::Vector3 {
				return worldRotation * (DirectX::XM_1DIVPI * 180.0f);
			};
			rotItem.dataProvider = [this](Math::Vector3 newValue) {
				worldRotation = newValue / (DirectX::XM_1DIVPI * 180.0f);
			};

			auto& sclItem = group->CreateWidget<UI::DragFloat3>("Scaling", worldScaling);
			sclItem.dataGatherer = [this]() -> Math::Vector3 {
				return worldScaling;
			};
			sclItem.dataProvider = [this](Math::Vector3 newValue) {
				worldScaling = newValue;
			};
			*/
		}

	};

}