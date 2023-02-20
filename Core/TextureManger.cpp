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
	* ͨ���û��Ĳ�����ע����Դ
	*/
	void TextureManger::RegisterResource(Texture* target) {
		Tool::OutputMemoryStream oStream;
		target->Serialize(oStream);

		std::string abPath = mAssetPath + "/" + target->GetName();
		std::ofstream oFile(abPath + ".tex", std::ios::binary | std::ios::out);
		oFile.write((const char*)oStream.Data(), oStream.Size());
	}

	/*
	* ͨ��ָ����·��(��������Ŀ·����������·��)��������������Դ.
	* ����·������Դ��AssetLoader��������ͨ��RegisterResource()������ע���������.
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