#include "Game/Game.h"
#include "Game/SystemManger.h"
#include "Core/ServiceLocator.h"
#include "Physics/PhysicsSystem.h"
#include "Renderer/RenderEngine.h"
#include "ECS/Entity.h"
#include "ECS/CCamera.h"
#include "ECS/CTransform.h"

namespace Game {

	Game::Game(Context& context) 
	: mContext(context) {
		mGameInitializer.DoInitialization();
	}

	Game::~Game() {
	}

	void Game::Run() {
		float delta = mContext.clock->GetDeltaTime();

		PreUpdate(delta);
		Update(delta);
		PostUpdate(delta);

		mContext.clock->Update();
	}

	void Game::Update(float delta) {
		// ʵ����Ϊ
		CORESERVICE(SystemManger).Run();

		// �������

		// ִ����Ⱦ
		ECS::Camera* editorCamera = nullptr;
		ECS::Transform* editorTransform = nullptr;
		ECS::Entity::Foreach([&](ECS::Camera& camera, ECS::Transform& transform) {
			if (camera.cameraType == ECS::CameraType::EditorCamera) {
				editorCamera = &camera;
				editorTransform = &transform;
			}
		});
		CORESERVICE(Renderer::RenderEngine).Update(
			mContext.clock->GetDeltaTime(), mContext.clock->GetTimeSinceStart(),
			*editorCamera, *editorTransform);
		CORESERVICE(Renderer::RenderEngine).Render();
	}

	void Game::PreUpdate(float delta) {
		mContext.inputManger->PreUpdate(delta);
	}

	void Game::PostUpdate(float delta) {
		mContext.inputManger->PostUpdate();
	}

}