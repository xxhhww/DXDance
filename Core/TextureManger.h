#pragma once
#include "IAssetManger.h"
#include "Texture.h"

namespace Core {
	class TextureManger : public IAssetManger<Texture> {
	public:
		/*
		* ͨ���û��Ĳ�����ע����Դ
		*/
		void RegisterResource(Texture* target) override;

		/*
		* ͨ��ָ����·��(��������Ŀ·����������·��)��������������Դ.
		* ����·������Դ��TextureLoader��������ͨ��RegisterResource()������ע���������.
		*/
		Texture* LoadResource(const std::string& path) override;
	};
}