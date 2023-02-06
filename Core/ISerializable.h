#pragma once
#include <rapidjson/document.h>

namespace Core {
	// 所有可以序列化为Json数据的类(一般是一些资产类，如材质、着色器等)的接口
	class IJsonSerializable {
	public:
		virtual ~IJsonSerializable() = default;
		virtual void Serialize(rapidjson::Document& outputDoc) = 0;
		virtual void Deserialize(const rapidjson::Document& inputDoc) = 0;
	};
}