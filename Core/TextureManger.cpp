#include "TextureManger.h"
#include <fstream>
#include "Tools/StrUtil.h"
#include "TextureLoader.h"

namespace Core {
	void Texture::Serialize(Tool::OutputMemoryStream& blob) const {

	}

	void Texture::Deserialize(const Tool::InputMemoryStream& blob) {

	}

	/*
	* 通过用户的操作来注册资源
	*/
	void TextureManger::RegisterResource(Texture* target) {
		Tool::OutputMemoryStream oStream;
		target->Serialize(oStream);

		std::string abPath = mAssetPath + "/" + target->GetName();
		std::ofstream oFile(abPath + ".tex", std::ios::binary | std::ios::out);
		oFile.write((const char*)oStream.Data(), oStream.Size());
	}

	/*
	* 通过指定的路径(必须是项目路径或者引擎路径)来解析并管理资源.
	* 其他路径的资源由AssetLoader解析，并通过RegisterResource()方法来注册进管理类.
	*/
	Texture* TextureManger::LoadResource(const std::string& path) {
		std::string name = Tool::StrUtil::RemoveBasePath(path);

		std::string abPath = mAssetPath + "/" + path;
		std::ifstream iFile(abPath + ".tex", std::ios::binary | std::ios::in);
		Tool::InputMemoryStream iStream(iFile);

		Texture* texture = new Texture();
		texture->SetName(name);
		texture->Deserialize(iStream);

		TextureLoader::Create(abPath, *texture);

		mAssets.emplace(texture->GetID(), texture);

		return texture;
	}
}