#pragma once
#include "IComponent.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

namespace Core {
	/*
	* ����ʱʹ�õ����
	*/
	class FooComponent : public IComponent {
	public:

		/*
		* ���л�Ϊ����������
		*/
		void SerializeBinary(Tool::OutputMemoryStream& blob) const override {
			blob.Write(a);
			blob.Write(b);
			blob.Write(c);
			blob.Write(d);
			blob.Write(e);
		}

		/*
		* �����л�����������
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