#pragma once
#include "IAssetManger.h"
#include "Texture.h"

namespace Core {
	class TextureManger : public IAssetManger<Texture> {
	public:
		/*
		* ���캯��
		*/
		TextureManger(AssetPathDataBase* dataBase, bool enableUnload);
	};
}