#pragma once
#include "IComponent.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Core {
	/*
	* 测试时使用的组件
	*/
	class FooComponent : public IComponent {
	public:

		/*
		* 序列化为二进制数据
		*/
		void SerializeBinary(Tool::OutputMemoryStream& blob) const override {
			blob.Write(a);
			blob.Write(b);
			blob.Write(c);
			blob.Write(d);
			blob.Write(e);
		}

		/*
		* 反序列化二进制数据
		*/
		void DeserializeBinary(Tool::InputMemoryStream& blob) override {
			blob.Read(a);
			blob.Read(b);
			blob.Read(c);
			blob.Read(d);
			blob.Read(e);
		}

	public:
		int				a;
		float			b;
		Math::Vector2	c;
		Math::Vector3	d;
		Math::Vector4	e;
	};
}