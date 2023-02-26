#pragma once
#include "IAssetManger.h"
#include "Texture.h"

namespace Core {
	class TextureManger : public IAssetManger<Texture> {
	public:
		/*
		* ���캯��
		*/
		TextureManger(const std::string& assetPath, const std::string& enginePath);

		/*
		* ͨ���û��Ĳ�����ע����Դ����ָ�뽻������ָ�����
		*/
		void RegisterResource(Texture* target) override;

		/*
		* 
		*/

		/*
		* ͨ��ָ����·��(��������Ŀ·����������·��)��������������Դ.
		* ����·������Դ��TextureLoader��������ͨ��RegisterResource()������ע���������.
		*/
		Texture* LoadResource(const std::string& path) override;
	};
}