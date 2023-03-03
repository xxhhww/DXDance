#pragma once
#include "IAssetManger.h"
#include "Texture.h"

namespace Core {
	class TextureManger : public IAssetManger<Texture> {
	public:
		/*
		* 构造函数
		*/
		TextureManger(const std::string& assetPath, const std::string& enginePath);

		/*
		* 通过用户的操作来注册资源，将指针交给智能指针管理
		*/
		void RegisterResource(Texture* target) override;
	};
}