#pragma once
#include "MemoryStream.h"
#include "JsonHelper.h"
#include "SerializeHepler.h"

namespace Tool {

	/*
	* ��ת�����л�
	*/
	class ISerializable {
	public:
		virtual ~ISerializable() = default;

		/*
		* ���л�Ϊ����������
		*/
		virtual void SerializeBinary(OutputMemoryStream& blob) {}

		/*
		* �����л�����������
		*/
		virtual void DeserializeBinary(InputMemoryStream& blob) {}

		/*
		* ���л�ΪJson����
		*/
		virtual void SerializeJson(JsonWriter& writer) const {}

		/*
		* �����л�Json����
		*/
		virtual void DeserializeJson(const JsonReader& reader) {}
	};
}