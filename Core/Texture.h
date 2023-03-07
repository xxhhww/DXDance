#pragma once
#include "IAsset.h"
#include "IAssetManger.h"
#include "DirectXTex/DirectXTex.h"

namespace Core {
	/*
	* �����ʲ�
	*/
	class Texture : public IAsset, public DirectX::ScratchImage {
	public:
		/*
		* ���캯��
		*/
		Texture(IAssetManger<Texture>* manger);

		/*
		* Ĭ����������
		*/
		~Texture() = default;

		/*
		* ����
		*/
		void Load(const std::string& path, bool aSync = false) override;

		/*
		* ж��
		*/
		void Unload(const std::string& path) override;

		inline void RemoveSRGB() {
			m_metadata.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		}

	private:
		IAssetManger<Texture>* mManger{ nullptr };
	};
}