#include "SceneManger.h"

#include "Renderer/CCamera.h"
#include "Renderer/CTransform.h"

#include "Tools/StrUtil.h"
#include "Tools/Assert.h"
#include "Tools/UIDGenerator.h"

namespace Core {
	SceneManger::SceneManger(const std::string& assetPath, const std::string& enginePath, AssetPathDataBase* pathDataBase)
	: mAssetPath(assetPath)
	, mEnginePath(enginePath)
	, mAssetPathDataBase(pathDataBase)
	, mCurrScene(nullptr) {}

	SceneManger::~SceneManger() {
		if (mCurrScene != nullptr) {
			UnLoadCurrentScene();
		}
	}

	void SceneManger::UnLoadCurrentScene() {
		if (mCurrScene == nullptr) return;
		mCurrScene->SaveToDisk();
		delete mCurrScene;
	}
	
	void SceneManger::SaveCurrentScene() {
		if (mCurrScene == nullptr) return;
		mCurrScene->SaveToDisk();
	}

	void SceneManger::CreateEmptyScene(const std::string& path) {
		if (mCurrScene != nullptr) {
			UnLoadCurrentScene();
		}
		mCurrScene = new Scene(this, Tool::UIDGenerator::Get());
		Actor* mainCamera = mCurrScene->CreateActor("MainCamera");
		auto& cTransform = mainCamera->GetComponent<Renderer::Transform>();
		cTransform.worldPosition = Math::Vector3{ 0.0f, 0.0f, -3.0f };

		auto& cCamera = mainCamera->AddComponent<Renderer::Camera>();
		cCamera.cameraType = Renderer::CameraType::RenderCamera;
		cCamera.mainCamera = true;

		mAssetPathDataBase->SetPath(mCurrScene->GetUID(), path);

		mCurrScene->SaveToDisk();
	}

	void SceneManger::LoadSceneFromDisk(const std::string& path) {

	}

	std::string SceneManger::GetFullPath(const std::string& path) {
		// 如果相对路径以 "Engine" 开头，则是引擎文件
		if (Tool::StrUtil::StartWith(path, "Engine")) {
			return mEnginePath + '\\' + path;
		}
		else if (Tool::StrUtil::StartWith(path, "Assets")) {
			return mAssetPath + '\\' + path;
		}
		else {
			ASSERT_FORMAT(false, "Unsupport File Path");
		}

		return path;
	}

	std::string SceneManger::GetFullPath(int64_t uid) {
		std::string path = mAssetPathDataBase->GetPath(uid);
		return GetFullPath(path);
	}

}