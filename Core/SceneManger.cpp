#include "SceneManger.h"

namespace Core {
	SceneManger::SceneManger(AssetPathDataBase* dataBase, const std::string& assetPath, const std::string& enginePath, bool enableUnload)
	: IAssetManger<Scene>(dataBase, assetPath, enginePath, enableUnload) 
	, mCurrScene(nullptr) {}

	SceneManger::~SceneManger() {

	}

	void SceneManger::UnLoadCurrentScene() {
		if (mCurrScene == nullptr) {
			return;
		}

		// 卸载当前场景
		UnuseResource(mCurrScene);
	}
	
	void SceneManger::CreateEmptyScene() {

	}

	void SceneManger::LoadSceneFromDisk(const std::string& path) {
		// 卸载当前场景
		UnLoadCurrentScene();

		// 加载当前场景
		mCurrScene = UseResource(path);
	}
}