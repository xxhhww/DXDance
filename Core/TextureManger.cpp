#include "TextureManger.h"

namespace Core {
	TextureManger::TextureManger(AssetPathDataBase* dataBase, bool enableUnload)
	: IAssetManger<Texture>(dataBase, enableUnload) {}


}