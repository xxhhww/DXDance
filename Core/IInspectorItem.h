#pragma once
#include "Tools/ISerializable.h"

namespace Core {

	/*
	* ����ʾ�����Դ�����
	*/
	class IInspectorItem : public Tool::ISerializable {
	public:

	private:
		void SerializeBinary(Tool::OutputMemoryStream& blob) const override {}
		void DeserializeBinary(const Tool::InputMemoryStream& blob) override {}
	};

}