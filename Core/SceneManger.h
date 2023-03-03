#pragma once
#include "IAssetManger.h"
#include "Scene.h"

namespace Core {
	/*
	* 场景管理器
	* 场景管理器不会在一个开始就将全部场景读取到内存中，而是只从硬盘中读取当前使用的场景
	* 对于不使用的场景，管理器会将它从内存中析构，该场景使用的Asset也会引用计数减一，
	*/
	class SceneManger : public IAssetManger<Scene> {
	public:

	private:
		Scene* mCurrScene{ nullptr };	// 当前正在处理的场景
	};
}