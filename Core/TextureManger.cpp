#include "TextureManger.h"

namespace Core {
	void Texture::Serialize(Tool::OutputMemoryStream& blob) const {

	}

	void Texture::Deserialize(const Tool::InputMemoryStream& blob) {

	}

	/*
	* ͨ��ָ����·��(��������Ŀ·����������·��)��������������Դ.
	* ����·������Դ��AssetLoader��������ͨ��RegisterResource()������ע���������.
	*/
	Texture* CreateResource(const std::string& path) {

	}
}