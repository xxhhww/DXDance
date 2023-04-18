#pragma once
#include "ECS/IComponent.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Core {
	/*
	* 测试时使用的组件
	*/
	class FooComponent : public ECS::IComponent {
	public:
		/*
		* 序列化为Json数据
		*/
		void SerializeJson(Tool::JsonWriter& writer) const override {
			using namespace Tool;

			writer.StartObject();

			SerializeHelper::SerializeString(writer, "Typename", std::string(typeid(FooComponent).name()));
			SerializeHelper::SerializeInt32(writer, "A", a);
			SerializeHelper::SerializeFloat(writer, "B", b);
			SerializeHelper::SerializeVector2(writer, "C", c);
			SerializeHelper::SerializeVector3(writer, "D", d);
			SerializeHelper::SerializeVector4(writer, "E", e);

			writer.EndObject();
		}

		/*
		* 反序列化Json数据
		*/
		void DeserializeJson(const Tool::JsonReader& reader) override {
			using namespace Tool;

			SerializeHelper::DeserializeInt32(reader, "A", a);
			SerializeHelper::DeserializeFloat(reader, "B", b);
			SerializeHelper::DeserializeVector2(reader, "C", c);
			SerializeHelper::DeserializeVector3(reader, "D", d);
			SerializeHelper::DeserializeVector4(reader, "E", e);
		}

	public:
		int				a;
		float			b;
		Math::Vector2	c;
		Math::Vector3	d;
		Math::Vector4	e;
	};
}