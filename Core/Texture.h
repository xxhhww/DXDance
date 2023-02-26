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
		* ���캯���������ʲ��ڱ༭������ʱ�������龰����Ҫ�ṩ�ʲ�����
		*/
		inline Texture(const std::string& name) : IAsset(name) {}

		/*
		* Ĭ����������
		*/
		inline ~Texture() = default;

		inline void RemoveSRGB() {
			m_metadata.format = DXGI_FORMAT_B8G8R8A8_UNORM;
		}

		void Serialize(Tool::OutputMemoryStream& blob) const	override;
		void Deserialize(const Tool::InputMemoryStream& blob)	override;
	};
}