#include "SceneView.h"

#include "Core/ServiceLocator.h"
#include "Core/SceneManger.h"
#include "Core/Scene.h"

#include "UI/Image.h"

#include "Windows/InputManger.h"

namespace App {
	SceneView::SceneView(
		const std::string& title,
		bool opend,
		const UI::PanelWindowSettings& panelSetting)
	: UI::PanelWindow(title, opend, panelSetting)
	, mFinalOutputRect(1920.0f, 1080.0f)
	, mRenderEngine(nullptr, mFinalOutputRect.x, mFinalOutputRect.y, 3u) {
		mBackImage = &CreateWidget<UI::Image>(0u, mFinalOutputRect);
		mSceneManger = &CORESERVICE(Core::SceneManger);
		LoadNewScene("Undefined");
	}

	void SceneView::BindHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
		mRenderEngine.BindFinalOuputSRV(cpuHandle);
		mBackImage->textureID = gpuHandle.ptr;
	}

	void SceneView::LoadNewScene(const std::string& path) {
		mSceneManger->CreateEmptyScene("Undefined");
		mCurrentScene = mSceneManger->GetCurrentScene();
		mEditorCamera = &mCurrentScene->editorCamera;
		mEditorTransform = &mCurrentScene->editorTransform;
	}

	void SceneView::Update(float dt) {
		mAvailableSize = GetAvailableSize();
		mBackImage->size = mAvailableSize;
		// ImGui
		if (IsFocused() && CORESERVICE(Windows::InputManger).IsMouseButtonPressed(Windows::EMouseButton::MOUSE_RBUTTON)) {
			// ����������ƶ�
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_W)) {
				XMVECTOR s = XMVectorReplicate(dt * mCurrentScene->editorCamera.translationSpeed);
				XMVECTOR l = XMLoadFloat3(&mEditorCamera->lookUp);
				XMVECTOR p = XMLoadFloat3(&mEditorTransform->worldPosition);
				XMStoreFloat3(&mEditorTransform->worldPosition, XMVectorMultiplyAdd(s, l, p));
			}
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_S)) {
				XMVECTOR s = XMVectorReplicate(-dt * mEditorCamera->translationSpeed);
				XMVECTOR l = XMLoadFloat3(&mEditorCamera->lookUp);
				XMVECTOR p = XMLoadFloat3(&mEditorTransform->worldPosition);
				XMStoreFloat3(&mEditorTransform->worldPosition, XMVectorMultiplyAdd(s, l, p));
			}
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_A)) {
				XMVECTOR s = XMVectorReplicate(-dt * mEditorCamera->translationSpeed);
				XMVECTOR r = XMLoadFloat3(&mEditorCamera->right);
				XMVECTOR p = XMLoadFloat3(&mEditorTransform->worldPosition);
				XMStoreFloat3(&mEditorTransform->worldPosition, XMVectorMultiplyAdd(s, r, p));
			}
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_D)) {
				XMVECTOR s = XMVectorReplicate(dt * mEditorCamera->translationSpeed);
				XMVECTOR r = XMLoadFloat3(&mEditorCamera->right);
				XMVECTOR p = XMLoadFloat3(&mEditorTransform->worldPosition);
				XMStoreFloat3(&mEditorTransform->worldPosition, XMVectorMultiplyAdd(s, r, p));
			}
			// �����������ת
			if (CORESERVICE(Windows::InputManger).IsMouseMove()) {
				Math::Vector2 rawDelta = CORESERVICE(Windows::InputManger).GetMouseRawDelta();

				XMMATRIX R = XMMatrixRotationAxis(XMLoadFloat3(&mEditorCamera->right), mEditorCamera->rotationSpeed * XMConvertToRadians((float)rawDelta.y));
				XMStoreFloat3(&mEditorCamera->up, XMVector3TransformNormal(XMLoadFloat3(&mEditorCamera->up), R));
				XMStoreFloat3(&mEditorCamera->lookUp, XMVector3TransformNormal(XMLoadFloat3(&mEditorCamera->lookUp), R));

				XMMATRIX R1 = XMMatrixRotationY(mEditorCamera->rotationSpeed * XMConvertToRadians((float)rawDelta));

				XMStoreFloat3(&mEditorCamera->right, XMVector3TransformNormal(XMLoadFloat3(&mEditorCamera->right), R1));
				XMStoreFloat3(&mEditorCamera->up, XMVector3TransformNormal(XMLoadFloat3(&mEditorCamera->up), R1));
				XMStoreFloat3(&mEditorCamera->lookUp, XMVector3TransformNormal(XMLoadFloat3(&mEditorCamera->lookUp), R1));
			}
		}

		// ���±༭���������Ⱦ������ľ�������
		auto updateCameraMatrix = [](Renderer::Camera& camera, Renderer::Transform& transform) {
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
		};

		updateCameraMatrix(*mEditorCamera, *mEditorTransform);
		ECS::Entity::Foreach([&](Renderer::Camera& camera, Renderer::Transform& transform) {
			updateCameraMatrix(camera, transform);
		});
	}

	void SceneView::Render(float dt) {
		// ����PerFrameData
		mRenderEngine.Update(dt, *mEditorCamera, *mEditorTransform);
		// ��Ⱦ
		mRenderEngine.Render();
	}

	Math::Vector2 SceneView::GetAvailableSize() const {
		Math::Vector2 result = GetSize() - Math::Vector2{ 0.0f, 25.0f }; // 25 == title bar height
		return result;
	}

}