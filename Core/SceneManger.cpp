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

		// ж�ص�ǰ����
		UnuseResource(mCurrScene);
	}
	
	void SceneManger::CreateEmptyScene() {

	}

	void SceneManger::LoadSceneFromDisk(const std::string& path) {
		// ж�ص�ǰ����
		UnLoadCurrentScene();

		// ���ص�ǰ����
		mCurrScene = UseResource(path);
	}
}