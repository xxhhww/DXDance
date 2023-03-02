#pragma once
#include "Tools/ISerializable.h"

namespace Core {

	/*
	* 可显示在属性窗口上
	*/
	class IInspectorItem : public Tool::ISerializable {
	public:

	private:
		void SerializeBinary(Tool::OutputMemoryStream& blob) const override {}
		void DeserializeBinary(const Tool::InputMemoryStream& blob) override {}
	};

}