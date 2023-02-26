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

	Texture* TextureManger::LoadResource(const std::string& path) {
		std::string name = Tool::StrUtil::RemoveBasePath(path);

		std::string abPath = mAssetPath + "/" + path;
		std::ifstream iFile(abPath + ".meta", std::ios::binary | std::ios::in);
		Tool::InputMemoryStream iStream(iFile);

		Texture* texture = new Texture();
		texture->SetName(name);
		texture->Deserialize(iStream);

		TextureLoader::Create(abPath, *texture);

		mAssets.emplace(texture->GetUID(), texture);

		return texture;
	}
}