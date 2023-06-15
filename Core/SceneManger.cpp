#include "SceneManger.h"

#include "Core/EditorAssetManger.h"
#include "Core/ServiceLocator.h"

#include "ECS/CCamera.h"
#include "ECS/CTransform.h"
#include "ECS/CMeshRenderer.h"

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
		mCurrScene = nullptr;
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
		mCurrScene->editorTransform.worldPosition = Math::Vector3{ 0.0f, 0.0f, -3.0f };

		Actor* mainCamera = mCurrScene->CreateActor("MainCamera");
		auto& cTransform = mainCamera->GetComponent<ECS::Transform>();
		auto& cCamera = mainCamera->AddComponent<ECS::Camera>();
		cCamera.cameraType = ECS::CameraType::RenderCamera;
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