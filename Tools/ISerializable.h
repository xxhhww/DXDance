#pragma once
#include "MemoryStream.h"
#include "rapidjson/document.h"

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
		virtual void SerializeBinary(OutputMemoryStream& blob) const = 0;

		/*
		* 反序列化二进制数据
		*/
		virtual void DeserializeBinary(const InputMemoryStream& blob) = 0;

		/*
		* 序列化为Json数据
		*/
		virtual void SerializeJson(rapidjson::Document& doc) const = 0;

		/*
		* 反序列化Json数据
		*/
		virtual void DeserializeJson(const rapidjson::Document& doc) = 0;
	};
}