#pragma once
#include <rapidjson/document.h>

namespace Core {
	// ���п������л�ΪJson���ݵ���(һ����һЩ�ʲ��࣬����ʡ���ɫ����)�Ľӿ�
	class IJsonSerializable {
	public:
		virtual ~IJsonSerializable() = default;
		virtual void Serialize(rapidjson::Document& outputDoc) = 0;
		virtual void Deserialize(const rapidjson::Document& inputDoc) = 0;
	};
}