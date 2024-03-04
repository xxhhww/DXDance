#include "Game/CameraSystem.h"
#include "Game/GlobalSetting.h"
#include "Game/CTank.h"

#include "ECS/Entity.h"
#include "ECS/CCamera.h"
#include "ECS/CTransform.h"

#include "Core/ServiceLocator.h"

#include "Tools/Clock.h"

#include "Windows/InputManger.h"

namespace Game {
	/*			    Camera
	*				  |\
	* TopViewHeight-> | \
	*				  |  \ Player
	*/
	inline static constexpr float sCameraTopViewHeight = 15.0f;	// ��������Ӹ߶�

	void CameraSystem::Create() {

		Math::Vector3 playerPosition = CORESERVICE(Game::GlobalSetting).playerPosition;

		// ������������ֵ�λ��(��TankΪ���ĵİ���������ϵ���߶Ƚ�45�� �߶�50�� �뾶50����2)
		Math::Vector3 cameraPosition{ playerPosition.x, playerPosition.y + sCameraTopViewHeight,  playerPosition.z - sCameraTopViewHeight };
		Math::Quaternion cameraRotation{};

		// RenderCamera(���������)
		{
			auto renderCameraEntity = ECS::Entity::Create<ECS::Transform, ECS::Camera>();

			auto& renderCameraTransform = renderCameraEntity.GetComponent<ECS::Transform>();
			renderCameraTransform.worldPosition = cameraPosition;
			renderCameraTransform.worldRotation = cameraRotation;

			auto& renderCameraComponent = renderCameraEntity.GetComponent<ECS::Camera>();
			renderCameraComponent.cameraType = ECS::CameraType::RenderCamera;
			renderCameraComponent.mainCamera = true;
			renderCameraComponent.frustum.farZ = 5000.0f;
		}

		// EditorCamera(FPS�����)
		{
			auto editorCameraEntity = ECS::Entity::Create<ECS::Transform, ECS::Camera>();

			auto& editorCameraTransform = editorCameraEntity.GetComponent<ECS::Transform>();
			editorCameraTransform.worldPosition = cameraPosition;
			editorCameraTransform.worldRotation = cameraRotation;

			auto& editorCameraComponent = editorCameraEntity.GetComponent<ECS::Camera>();
			editorCameraComponent.translationSpeed *= 5.0f;
			editorCameraComponent.cameraType = ECS::CameraType::EditorCamera;
			editorCameraComponent.mainCamera = false;
			editorCameraComponent.frustum.farZ = 5000.0f;
		}
	}

	void CameraSystem::Destory() {
	}

	void CameraSystem::PrePhysicsUpdate() {
		bool isPaused = CORESERVICE(Game::GlobalSetting).isPaused;
		float dt = CORESERVICE(Tool::Clock).GetDeltaTime();

		// ����������ƶ�
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

		// �����������ת
		auto rotateCamera = [&](ECS::Camera& camera, ECS::Transform& transform) {
			// �����������ת
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

		// ����������ľ�������
		auto updateCameraMatrix = [&](ECS::Camera& camera, ECS::Transform& transform) {
			// ����ViewMatrix
			const XMVECTOR camTarget = transform.worldPosition + camera.lookUp;
			camera.viewMatrix = XMMatrixLookAtLH(
				transform.worldPosition,
				camTarget,
				XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f));

			// ����ProjMatrix
			camera.projMatrix = XMMatrixPerspectiveFovLH(
				camera.frustum.fovY,
				camera.frustum.aspect,
				camera.frustum.nearZ,
				camera.frustum.farZ);

			// ����VP����
			camera.viewProjMatrix = camera.viewMatrix * camera.projMatrix;
		};

		ECS::Entity::Foreach([&](ECS::Camera& camera, ECS::Transform& transform) {
			// �����������Ϊ

			// ��Ϸ����ʱ�����������������Tank�ƶ����ƶ�
			if (!isPaused) {
				rotateCamera(camera, transform);
			}

			// ��Ϸ��ͣʱ���༭��������������ƶ�
			if (isPaused && camera.cameraType == ECS::CameraType::EditorCamera) {
				// �༭�������Ҫ��ס�Ҽ�
				if (CORESERVICE(Windows::InputManger).IsMouseButtonPressed(Windows::EMouseButton::MOUSE_RBUTTON)) {
					moveCamera(camera, transform);
					rotateCamera(camera, transform);
				}
			}

			// �������������
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