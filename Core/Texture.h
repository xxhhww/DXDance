#pragma once
#include "IAsset.h"
#include "ThirdParty/include/DirectXTex/DirectXTex.h"

namespace Core {
	/*
	* �����ʲ�
	*/
	class Texture : public IAsset, public DirectX::ScratchImage {
	public:
		/*
		* Ĭ�Ϲ��캯���������ʲ����ļ��ж�ȡ���龰
		*/
		inline Texture() = default;

		/*
		* Ĭ����������
		*/
		inline ~Texture() = default;

		inline void RemoveSRGB() {
			m_metadata.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		}

		/*
		* ���л�Ϊ����������
		*/
		virtual void SerializeBinary(Tool::OutputMemoryStream& blob) const {}

		/*
		* �����л�����������
		*/
		virtual void DeserializeBinary(const Tool::InputMemoryStream& blob) {}

		/*
		* ���л�ΪJson����
		*/
		virtual void SerializeJson(rapidjson::Document& doc) const {}

		/*
		* �����л�Json����
		*/
		virtual void DeserializeJson(const rapidjson::Document& doc) {}
	};
}