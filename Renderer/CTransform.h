#pragma once
#include "ECS/IComponent.h"

#include "Math/Vector.h"
#include "Math/Quaternion.h"
#include "Math/Matrix.h"

namespace Renderer {

	class Transform : public ECS::IComponent {
	public:
		Math::Vector3		worldPosition;	// 相对于世界原点的位置
		Math::Quaternion	worldRotation;	// 相对于世界原点的旋转
		Math::Vector3		worldScale;		// 相对于世界原点的缩放
		Math::Matrix4		worldMatrix;	// 相对于世界原点的变换

	public:
		/*
		* 序列化为Json数据
		*/
		void SerializeJson(Tool::JsonWriter& writer) const override;

		/*
		* 反序列化Json数据
		*/
		void DeserializeJson(const Tool::JsonReader& reader) override;
	};

}