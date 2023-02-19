#include "TextureManger.h"

namespace Core {
	void Texture::Serialize(Tool::OutputMemoryStream& blob) const {

	}

	void Texture::Deserialize(const Tool::InputMemoryStream& blob) {

	}

	/*
	* 通过指定的路径(必须是项目路径或者引擎路径)来解析并管理资源.
	* 其他路径的资源由AssetLoader解析，并通过RegisterResource()方法来注册进管理类.
	*/
	Texture* CreateResource(const std::string& path) {

	}
}