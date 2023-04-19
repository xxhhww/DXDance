#include "SceneView.h"

#include "Core/ServiceLocator.h"
#include "Core/SceneManger.h"

#include "UI/Image.h"

#include "Windows/InputManger.h"

namespace App {
	SceneView::SceneView(
		const std::string& title,
		bool opend,
		const UI::PanelWindowSettings& panelSetting)
	: UI::PanelWindow(title, opend, panelSetting)
	, mFinalOutputRect(1920.0f, 1080.0f)
	, mRenderEngine(nullptr, mFinalOutputRect.x, mFinalOutputRect.y) {
		mBackImage = &CreateWidget<UI::Image>(0u, mFinalOutputRect);
		mSceneManger = &CORESERVICE(Core::SceneManger);
		mSceneManger->CreateEmptyScene("Undefined");

		mTransformComponent.worldPosition = Math::Vector3{ 0.0f, 0.0f, -3.0f };
	}

	void SceneView::BindHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
		mRenderEngine.BindFinalOuputSRV(cpuHandle);
		mBackImage->textureID = gpuHandle.ptr;
	}

	void SceneView::Update(float dt) {
		mAvailableSize = GetAvailableSize();
		mBackImage->size = mAvailableSize;
		// ImGui
		if (IsFocused() && CORESERVICE(Windows::InputManger).IsMouseButtonPressed(Windows::EMouseButton::MOUSE_RBUTTON)) {
			// 处理摄像机移动
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_W)) {
				XMVECTOR s = XMVectorReplicate(dt * mCameraComponent.translationSpeed);
				XMVECTOR l = XMLoadFloat3(&mCameraComponent.lookUp);
				XMVECTOR p = XMLoadFloat3(&mTransformComponent.worldPosition);
				XMStoreFloat3(&mTransformComponent.worldPosition, XMVectorMultiplyAdd(s, l, p));
				OutputDebugString(L"WWW\n");
			}
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_S)) {
				XMVECTOR s = XMVectorReplicate(-dt * mCameraComponent.translationSpeed);
				XMVECTOR l = XMLoadFloat3(&mCameraComponent.lookUp);
				XMVECTOR p = XMLoadFloat3(&mTransformComponent.worldPosition);
				XMStoreFloat3(&mTransformComponent.worldPosition, XMVectorMultiplyAdd(s, l, p));
				OutputDebugString(L"SSS\n");
			}
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_A)) {

			}
			if (CORESERVICE(Windows::InputManger).IsKeyDown(Windows::EKey::KEY_D)) {

			}
			// 处理摄像机旋转

		}

		// 更新编辑摄像机与渲染摄像机的矩阵数据
		auto updateCameraMatrix = [](Renderer::Camera& camera, Renderer::Transform& transform) {
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
		};

		updateCameraMatrix(mCameraComponent, mTransformComponent);
		ECS::Entity::Foreach([&](Renderer::Camera& camera, Renderer::Transform& transform) {
			updateCameraMatrix(camera, transform);
		});
	}

	void SceneView::Render(float dt) {
		// 更新PerFrameData
		mRenderEngine.Update(dt, mCameraComponent, mTransformComponent);
		// 渲染
		mRenderEngine.Render();
	}

	Math::Vector2 SceneView::GetAvailableSize() const {
		Math::Vector2 result = GetSize() - Math::Vector2{ 0.0f, 25.0f }; // 25 == title bar height
		return result;
	}

}