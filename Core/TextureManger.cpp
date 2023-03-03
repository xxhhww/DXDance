#include "TextureManger.h"
#include <fstream>
#include "Tools/StrUtil.h"
#include "TextureLoader.h"

namespace Core {
	TextureManger::TextureManger(const std::string & assetPath, const std::string & enginePath)
	: IAssetManger<Texture>(assetPath, enginePath) {}

	void TextureManger::RegisterResource(Texture* target) {
		mAssets.emplace(target->GetUID(), target);

		int i = 32;
	}
}