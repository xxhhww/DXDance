#pragma once
#include "IAssetManger.h"
#include "Texture.h"

namespace Core {
	class TextureManger : public IAssetManger<Texture> {
	public:
		/*
		* ͨ��ָ����·��(��������Ŀ·����������·��)��������������Դ.
		* ����·������Դ��AssetLoader��������ͨ��RegisterResource()������ע���������.
		*/
		Texture* CreateResource(const std::string& path) override;
	};
}