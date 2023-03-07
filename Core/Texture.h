#pragma once
#include "IAsset.h"
#include "IAssetManger.h"
#include "DirectXTex/DirectXTex.h"

namespace Core {
	/*
	* 纹理资产
	*/
	class Texture : public IAsset, public DirectX::ScratchImage {
	public:
		/*
		* 构造函数
		*/
		Texture(IAssetManger<Texture>* manger);

		/*
		* 默认析构函数
		*/
		~Texture() = default;

		/*
		* 加载
		*/
		void Load(const std::string& path, bool aSync = false) override;

		/*
		* 卸载
		*/
		void Unload(const std::string& path) override;

		inline void RemoveSRGB() {
			m_metadata.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		}

	private:
		IAssetManger<Texture>* mManger{ nullptr };
	};
}