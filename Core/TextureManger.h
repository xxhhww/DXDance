#pragma once
#include "IAssetManger.h"
#include "Texture.h"

namespace Core {
	class TextureManger : public IAssetManger<Texture> {
	public:
		/*
		* ¹¹Ôìº¯Êý
		*/
		TextureManger(AssetPathDataBase* dataBase, bool enableUnload);
	};
}