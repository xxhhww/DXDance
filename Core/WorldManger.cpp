#include "Core/WorldManger.h"
#include "ECS/CCamera.h"
#include "ECS/CTransform.h"
#include "ECS/CMeshRenderer.h"
#include "ECS/CSky.h"

namespace Core {

	WorldManger::WorldManger() {
	}

	WorldManger::~WorldManger() {
		if (mWorld != nullptr) {
			delete mWorld;
		}
	}

	void WorldManger::CreateEmptyWorld() {

		mWorld = new World(this);

		// MainCamera(RenderCamera)
		{
			Actor* cameraActor = mWorld->CreateActor("MainCamera");
			auto& camera = cameraActor->AddComponent<ECS::Camera>();
			camera.cameraType = ECS::CameraType::RenderCamera;
			camera.mainCamera = true;
		}
		// Sky
		{
			Actor* skyActor = mWorld->CreateActor("Sky");
			auto& transform = skyActor->GetComponent<ECS::Transform>();
			transform.worldRotation = Math::Vector3{ DirectX::XM_PIDIV2, 0.0f, 0.0f };
			auto& sky = skyActor->AddComponent<ECS::Sky>();
		}

	}

}