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
		* 构造函数，用于资产在编辑器运行时创建的情景，需要提供资产名称
		*/
		inline Texture(const std::string& name) : IAsset(name) {}

		/*
		* 默认析构函数
		*/
		inline ~Texture() = default;

		inline void RemoveSRGB() {
			m_metadata.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		}

		void Serialize(Tool::OutputMemoryStream& blob) const	override;
		void Deserialize(const Tool::InputMemoryStream& blob)	override;
	};
}