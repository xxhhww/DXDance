#pragma once
#include "IAssetManger.h"
#include "Texture.h"

namespace Core {
	class TextureManger : public IAssetManger<Texture> {
	public:
		/*
		* 通过用户的操作来注册资源
		*/
		void RegisterResource(Texture* target) override;

		/*
		* 通过指定的路径(必须是项目路径或者引擎路径)来解析并管理资源.
		* 其他路径的资源由TextureLoader解析，并通过RegisterResource()方法来注册进管理类.
		*/
		Texture* LoadResource(const std::string& path) override;
	};
}