#include "Editor/SceneView.h"
#include "Editor/Inspector.h"

#include "Core/ServiceLocator.h"
#include "Core/SceneManger.h"
#include "Core/Scene.h"
#include "Core/EditorAssetManger.h"

#include "UI/Image.h"
#include "UI/UIManger.h"

#include "Windows/InputManger.h"

namespace App {
	SceneView::SceneView(
		const std::string& title,
		bool opend,
		const UI::PanelWindowSettings& panelSetting)
	: UI::PanelWindow(title, opend, panelSetting) {
		mRenderEngine = &CORESERVICE(Renderer::RenderEngine);
		mBackImage = &CreateWidget<UI::Image>(0u, Math::Vector2{ 1920.0f, 1080.0f });
		mSceneManger = &CORESERVICE(Core::SceneManger);
		LoadNewScene("Undefined");

		RegisterEditorRenderPass();
	}

	void SceneView::BindHandle(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, D3D12_GPU_DESCRIPTOR_HANDLE gpuHandle) {
		mRenderEngine->BindFinalOuputSRV(cpuHandle);
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
			// 处理摄像机移动
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
			// 处理摄像机旋转
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

		// 更新编辑摄像机与渲染摄像机的矩阵数据
		auto updateCameraMatrix = [](ECS::Camera& camera, ECS::Transform& transform) {
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

		updateCameraMatrix(*mEditorCamera, *mEditorTransform);
		ECS::Entity::Foreach([&](ECS::Camera& camera, ECS::Transform& transform) {
			const float rt = fmod(transform.worldRotation.y, DirectX::XM_2PI);

			if (rt > DirectX::XM_PI)
				transform.worldRotation.y = rt - DirectX::XM_2PI;
			else if (rt < -DirectX::XM_PI)
				transform.worldRotation.y = rt + DirectX::XM_2PI;

			camera.lookUp = XMVector4Transform(Math::Vector4{ 0.0f, 0.0f, 1.0f, 0.0f }, XMMatrixRotationRollPitchYaw(transform.worldRotation.x, transform.worldRotation.y, transform.worldRotation.z));
			camera.right  = XMVector3Cross(Math::Vector4{ 0.0f, 1.0f, 0.0f, 0.0f }, camera.lookUp);
			camera.up     = XMVector3Cross(camera.lookUp, camera.right);

			updateCameraMatrix(camera, transform);
		});
	}

	void SceneView::Render(float dt) {
		HandleActorPicking();

		// 更新PerFrameData
		mRenderEngine->Update(dt, *mEditorCamera, *mEditorTransform);
		// 渲染
		mRenderEngine->Render();
	}

	Math::Vector2 SceneView::GetAvailableSize() const {
		Math::Vector2 result = GetSize() - Math::Vector2{ 0.0f, 25.0f }; // 25 == title bar height
		return result;
	}

	void SceneView::HandleActorPicking() {
		RenderSceneForActorPicking();

	}

	void SceneView::RenderSceneForActorPicking() {
		
		// Create D3D Object First
		CreateD3DObjectForPicking();

		// Render Actors For Picking

		auto& inspectorPanel = CORESERVICE(UI::UIManger).GetCanvas()->GetPanel<App::Inspector>("Inspector");
		if (inspectorPanel.GetFocusActor() != nullptr) {
			// Render Axis For Picking
			auto* focusActor     = inspectorPanel.GetFocusActor();
			auto& actorTransform = focusActor->GetComponent<ECS::Transform>();

			// 更新AxisPassData
			mAxisPassData.modelMatrix = Math::Matrix4(actorTransform.worldPosition, actorTransform.worldRotation, Math::Vector3{ 1.0f, 1.0f, 1.0f }).Transpose();
			mAxisPassData.viewMatrix  = mEditorCamera->viewMatrix.Transpose();
			mAxisPassData.projMatrix  = mEditorCamera->projMatrix.Transpose();

			// 
		}
	}

	void SceneView::RegisterEditorRenderPass() {
		mRenderEngine->mEditorRenderPass += 
		[this](Renderer::CommandListWrap& commandList, Renderer::RenderContext& renderContext) {
			auto* axisRenderShader = renderContext.shaderManger->GetShader<Renderer::GraphicsShader>("AxisRender");
			auto  finalOutputRTV   = static_cast<Renderer::Texture*>(renderContext.resourceStorage->GetResourceByName("FinalOutput")->resource)->GetRTDescriptor()->GetCpuHandle();
		
			D3D12_VIEWPORT viewPort{};
			viewPort.TopLeftX = 0.0f;
			viewPort.TopLeftY = 0.0f;
			viewPort.Width = 1920.0f;
			viewPort.Height = 1080.0f;

			D3D12_RECT rect{};
			rect.left = 0.0f;
			rect.top = 0.0f;
			rect.right = 1920.0f;
			rect.bottom = 1080.0f;

			commandList->D3DCommandList()->RSSetViewports(1u, &viewPort);
			commandList->D3DCommandList()->RSSetScissorRects(1u, &rect);

			commandList->D3DCommandList()->OMSetRenderTargets(1u, &finalOutputRTV, false, nullptr);

			commandList->D3DCommandList()->SetGraphicsRootSignature(renderContext.shaderManger->GetBaseD3DRootSignature());
			commandList->D3DCommandList()->SetPipelineState(axisRenderShader->GetD3DPipelineState());

			auto dynamicAllocation = renderContext.dynamicAllocator->Allocate(sizeof(AxisPassData), 256u);
			memcpy(dynamicAllocation.cpuAddress, &mAxisPassData, sizeof(AxisPassData));

			commandList->D3DCommandList()->SetGraphicsRootConstantBufferView(0u, renderContext.resourceStorage->rootConstantsPerFrameAddress);
			commandList->D3DCommandList()->SetGraphicsRootConstantBufferView(1u, dynamicAllocation.gpuAddress);
		
			auto* axisMesh   = CORESERVICE(Core::EditorAssetManger).GetMesh("Arrow_Translate");
			auto  axisVBView = axisMesh->GetVertexBuffer()->GetVBDescriptor();
			auto  axisIBView = axisMesh->GetIndexBuffer()->GetIBDescriptor();
			commandList->D3DCommandList()->IASetVertexBuffers(0u, 1u, &axisVBView);
			commandList->D3DCommandList()->IASetIndexBuffer(&axisIBView);
			commandList->D3DCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandList->D3DCommandList()->DrawIndexedInstanced(axisIBView.SizeInBytes / sizeof(uint32_t), 3u, 0u, 0u, 0u);
		};
	}

	void SceneView::CreateD3DObjectForPicking() {
		// TODO 应对Resize
		if (mCommandListAllocator != nullptr) {
			return;
		}

		// Create D3D Object
		auto* device = mRenderEngine->mDevice.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();

		mCommandListAllocator = std::make_unique<GHL::CommandAllocator>(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		mPickingCommandList = std::make_unique<GHL::CommandList>(device, mCommandListAllocator->D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_DIRECT);
		mPickingFence = std::make_unique<GHL::Fence>(device);

		Renderer::TextureDesc pickingRTDesc{};
		pickingRTDesc.width  = mAvailableSize.x;
		pickingRTDesc.height = mAvailableSize.y;
		pickingRTDesc.format = DXGI_FORMAT_R8_UNORM;
		pickingRTDesc.initialState  = GHL::EResourceState::CopySource;
		pickingRTDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::CopySource;

		mPickingRenderTarget = std::make_unique<Renderer::Texture>(
			device,
			Renderer::ResourceFormat{ device, pickingRTDesc },
			descriptorAllocator,
			nullptr);

		Renderer::BufferDesc  pickingRBDesc{};
		pickingRBDesc.stride;
		pickingRBDesc.size = pickingRTDesc.width * pickingRTDesc.height * pickingRBDesc.stride;
		pickingRBDesc.usage = GHL::EResourceUsage::ReadBack;
		pickingRBDesc.initialState  = GHL::EResourceState::CopyDestination;
		pickingRBDesc.expectedState = GHL::EResourceState::CopyDestination;

		mPickingReadback = std::make_unique<Renderer::Buffer>(
			device,
			Renderer::ResourceFormat{ device, pickingRBDesc },
			descriptorAllocator,
			nullptr
		);
	}
}