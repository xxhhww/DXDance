#include "Game/CameraSystem.h"
#include "Game/GlobalData.h"
#include "Core/ServiceLocator.h"
#include "ECS/Entity.h"
#include "ECS/CCamera.h"
#include "ECS/CTransform.h"
#include "Tools/Clock.h"
#include "Windows/InputManger.h"

namespace Game {

	void CameraSystem::Create() {
	}

	void CameraSystem::Destory() {
	}

	void CameraSystem::PrePhysicsUpdate() {
		bool isPaused = CORESERVICE(GlobalData).isPaused;
		float dt = CORESERVICE(Tool::Clock).GetDeltaTime();

		// 处理摄像机移动
		auto moveCamera = [&](ECS::Camera& camera, ECS::Transform& transform) {
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_W)) {
				XMVECTOR s = XMVectorReplicate(dt * camera.translationSpeed);
				XMVECTOR l = XMLoadFloat3(&camera.lookUp);
				XMVECTOR p = XMLoadFloat3(&transform.worldPosition);
				XMStoreFloat3(&transform.worldPosition, XMVectorMultiplyAdd(s, l, p));
			}
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_S)) {
				XMVECTOR s = XMVectorReplicate(-dt * camera.translationSpeed);
				XMVECTOR l = XMLoadFloat3(&camera.lookUp);
				XMVECTOR p = XMLoadFloat3(&transform.worldPosition);
				XMStoreFloat3(&transform.worldPosition, XMVectorMultiplyAdd(s, l, p));
			}
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_A)) {
				XMVECTOR s = XMVectorReplicate(-dt * camera.translationSpeed);
				XMVECTOR r = XMLoadFloat3(&camera.right);
				XMVECTOR p = XMLoadFloat3(&transform.worldPosition);
				XMStoreFloat3(&transform.worldPosition, XMVectorMultiplyAdd(s, r, p));
			}
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_D)) {
				XMVECTOR s = XMVectorReplicate(dt * camera.translationSpeed);
				XMVECTOR r = XMLoadFloat3(&camera.right);
				XMVECTOR p = XMLoadFloat3(&transform.worldPosition);
				XMStoreFloat3(&transform.worldPosition, XMVectorMultiplyAdd(s, r, p));
			}
		};

		// 处理摄像机旋转
		auto rotateCamera = [&](ECS::Camera& camera, ECS::Transform& transform) {
			// 处理摄像机旋转
			if (CORESERVICE(Windows::InputManger).IsMouseMove()) {
				Math::Vector2 rawDelta = CORESERVICE(Windows::InputManger).GetMouseRawDelta();

				XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&camera.right), camera.rotationSpeed * XMConvertToRadians((float)rawDelta.y));
				XMStoreFloat3(&camera.up, XMVector3TransformNormal(XMLoadFloat3(&camera.up), R));
				XMStoreFloat3(&camera.lookUp, XMVector3TransformNormal(XMLoadFloat3(&camera.lookUp), R));

				XMMATRIX R1 = XMMatrixRotationY(camera.rotationSpeed * XMConvertToRadians((float)rawDelta));

				XMStoreFloat3(&camera.right, XMVector3TransformNormal(XMLoadFloat3(&camera.right), R1));
				XMStoreFloat3(&camera.up, XMVector3TransformNormal(XMLoadFloat3(&camera.up), R1));
				XMStoreFloat3(&camera.lookUp, XMVector3TransformNormal(XMLoadFloat3(&camera.lookUp), R1));
			}
		};

		// 更新摄像机的矩阵数据
		auto updateCameraMatrix = [&](ECS::Camera& camera, ECS::Transform& transform) {
			// 计算ViewMatrix
			const XMVECTOR camTarget = transform.worldPosition + camera.lookUp;
			camera.viewMatrix = XMMatrixLookAtLH(
				transform.worldPosition,
				camTarget,
				XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

			// 计算ProjMatrix
			camera.projMatrix = XMMatrixPerspectiveFovLH(
				camera.frustum.fovY,
				camera.frustum.aspect,
				camera.frustum.nearZ,
				camera.frustum.farZ);

			// 计算VP矩阵
			camera.viewProjMatrix = camera.viewMatrix * camera.projMatrix;
		};

		ECS::Entity::Foreach([&](ECS::Camera& camera, ECS::Transform& transform) {
			// 处理摄像机行为
			if (camera.cameraType == ECS::CameraType::RenderCamera && !isPaused) {
				// rotateCamera(camera, transform);
			}

			if (camera.cameraType == ECS::CameraType::EditorCamera) {
				// 编辑摄像机需要按住右键
				if (CORESERVICE(Windows::InputManger).IsMouseButtonPressed(Windows::EMouseButton::MOUSE_RBUTTON)) {
					moveCamera(camera, transform);
					rotateCamera(camera, transform);
				}
			}

			// 更新摄像机矩阵
			/*
			const float rt = fmod(transform.worldRotation.y, DirectX::XM_2PI);

			if (rt > DirectX::XM_PI)
				transform.worldRotation.y = rt - DirectX::XM_2PI;
			else if (rt < -DirectX::XM_PI)
				transform.worldRotation.y = rt + DirectX::XM_2PI;

			camera.lookUp = XMVector4Transform(Math::Vector4{ 0.0f, 0.0f, 1.0f, 0.0f }, XMMatrixRotationRollPitchYaw(transform.worldRotation.x, transform.worldRotation.y, transform.worldRotation.z));
			camera.right = XMVector3Cross(Math::Vector4{ 0.0f, 1.0f, 0.0f, 0.0f }, camera.lookUp);
			camera.up = XMVector3Cross(camera.lookUp, camera.right);
			*/
			updateCameraMatrix(camera, transform);
		});
	}

	void CameraSystem::PostPhysicsUpdate() {
	}

}