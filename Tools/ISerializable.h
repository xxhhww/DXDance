#pragma once
#include "MemoryStream.h"
#include "JsonHelper.h"
#include "SerializeHepler.h"

namespace Tool {

	/*
	* 可转换序列化
	*/
	class ISerializable {
	public:
		virtual ~ISerializable() = default;

		/*
		* 序列化为二进制数据
		*/
		virtual void SerializeBinary(OutputMemoryStream& blob) {}

		/*
		* 反序列化二进制数据
		*/
		virtual void DeserializeBinary(InputMemoryStream& blob) {}

		/*
		* 序列化为Json数据
		*/
		virtual void SerializeJson(JsonWriter& writer) const {}

		/*
		* 反序列化Json数据
		*/
		virtual void DeserializeJson(const JsonReader& reader) {}
	};
}