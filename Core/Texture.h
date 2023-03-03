#pragma once
#include "IAsset.h"
#include "ThirdParty/include/DirectXTex/DirectXTex.h"

namespace Core {
	/*
	* 纹理资产
	*/
	class Texture : public IAsset, public DirectX::ScratchImage {
	public:
		/*
		* 默认构造函数，用于资产从文件中读取的情景
		*/
		inline Texture() = default;

		/*
		* 默认析构函数
		*/
		inline ~Texture() = default;

		inline void RemoveSRGB() {
			m_metadata.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		}

		/*
		* 序列化为二进制数据
		*/
		virtual void SerializeBinary(Tool::OutputMemoryStream& blob) const {}

		/*
		* 反序列化二进制数据
		*/
		virtual void DeserializeBinary(const Tool::InputMemoryStream& blob) {}

		/*
		* 序列化为Json数据
		*/
		virtual void SerializeJson(rapidjson::Document& doc) const {}

		/*
		* 反序列化Json数据
		*/
		virtual void DeserializeJson(const rapidjson::Document& doc) {}
	};
}