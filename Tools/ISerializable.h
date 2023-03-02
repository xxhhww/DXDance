#pragma once
#include "MemoryStream.h"
#include "rapidjson/document.h"

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
		virtual void SerializeBinary(OutputMemoryStream& blob) const = 0;

		/*
		* �����л�����������
		*/
		virtual void DeserializeBinary(const InputMemoryStream& blob) = 0;

		/*
		* ���л�ΪJson����
		*/
		virtual void SerializeJson(rapidjson::Document& doc) const = 0;

		/*
		* �����л�Json����
		*/
		virtual void DeserializeJson(const rapidjson::Document& doc) = 0;
	};
}