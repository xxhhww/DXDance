#pragma once
#include "ECS/IComponent.h"

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/Matrix.h"

namespace ECS {

	class Transform : public ECS::IComponent {
	public:
		Math::Vector3	worldPosition;	// 相对于世界原点的位置
		Math::Vector3	worldRotation;	// 相对于世界原点的旋转
		Math::Vector3	worldScaling;	// 相对于世界原点的缩放
		Math::Matrix4	worldMatrix;	// 相对于世界原点的变换

	public:
		Transform()
		: worldPosition(0.0f, 0.0f, 0.0f)
		, worldRotation(0.0f, 0.0f, 0.0f)
		, worldScaling(1.0f, 1.0f, 1.0f) {}

	public:
		void SerializeJson(Tool::JsonWriter& writer) const override {
			using namespace Tool;

			writer.StartObject();

			SerializeHelper::SerializeString(writer, "Typename", std::string(typeid(Transform).name()));

			SerializeHelper::SerializeVector3(writer, "WorldPosition", worldPosition);
			SerializeHelper::SerializeVector3(writer, "WorldRotation", worldRotation);
			SerializeHelper::SerializeVector3(writer, "WorldScaling", worldScaling);
			SerializeHelper::SerializeMatrix(writer, "WorldMatrix", worldMatrix);

			writer.EndObject();
		}

		void DeserializeJson(const Tool::JsonReader& reader) override {
			using namespace Tool;

			// Typename由上层解析
			SerializeHelper::DeserializeVector3(reader, "WorldPosition", worldPosition);
			SerializeHelper::DeserializeVector3(reader, "WorldRotation", worldRotation);
			SerializeHelper::DeserializeVector3(reader, "WorldScaling", worldScaling);
			SerializeHelper::DeserializeMatrix(reader, "WorldMatrix", worldMatrix);
		}

		void OnInspector(UI::IWidgetContainer* container) override {
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
				return worldRotation;
			};
			rotItem.dataProvider = [this](Math::Vector3 newValue) {
				worldRotation = newValue;
			};

			auto& sclItem = group->CreateWidget<UI::DragFloat3>("Scaling", worldScaling);
			sclItem.dataGatherer = [this]() -> Math::Vector3 {
				return worldScaling;
			};
			sclItem.dataProvider = [this](Math::Vector3 newValue) {
				worldScaling = newValue;
			};
		}

	};

}