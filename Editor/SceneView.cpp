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
		mBackImage = &CreateWidget<UI::Image>(0u, Math::Vector2{ 979u, 635u });
		mSceneManger = &CORESERVICE(Core::SceneManger);

		mAxisMeshs.resize(std::underlying_type<EAxisOperation>::type(EAxisOperation::COUNT));
		mAxisMeshs[std::underlying_type<EAxisOperation>::type(EAxisOperation::TRANSLATE)] 
			= CORESERVICE(Core::EditorAssetManger).GetMesh("Arrow_Translate");
		mAxisMeshs[std::underlying_type<EAxisOperation>::type(EAxisOperation::ROTATE)]
			= CORESERVICE(Core::EditorAssetManger).GetMesh("Arrow_Rotate");
		mAxisMeshs[std::underlying_type<EAxisOperation>::type(EAxisOperation::SCALE)]
			= CORESERVICE(Core::EditorAssetManger).GetMesh("Arrow_Scale");

		mAxisBehavior = std::make_unique<AxisBehavior>();

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
		if (mAvailableSize.x <= 0.0f || mAvailableSize.y <= 0.0f) {
			return;
		}

		HandleActorPicking();

		// 更新PerFrameData
		mRenderEngine->Update(dt, *mEditorCamera, *mEditorTransform);
		// 渲染
		mRenderEngine->Render();
	}

	Math::Vector2 SceneView::GetAvailableSize() const {
		Math::Vector2 result = GetSize() - Math::Vector2{ 0.0f, 40.0f }; // 40 == 2 * title bar height
		return result;
	}

	void SceneView::HandleActorPicking() {
		if (CORESERVICE(Windows::InputManger).IsMouseButtonPressed(Windows::EMouseButton::MOUSE_RBUTTON)) {
			// 鼠标右键按下，说明在游览状态
			// 游览状态下不处理Picking
			return;
		}

		auto [mouseX, mouseY] = CORESERVICE(Windows::InputManger).GetMousePosition();
		mouseY -= 40.0f;	// 见GetAvailableSize()，这40是2 * Tile Bar
		mouseX -= mPosition.x;
		// 这里的两个7.0f是指backImage与SceneView之间的空档
		mouseY -= 7.0f;
		mouseX -= 7.0f;

		if (mouseX < 0 || mouseY < 0) {
			return;
		}

		if (CORESERVICE(Windows::InputManger).IsMouseButtonReleased(Windows::EMouseButton::MOUSE_LBUTTON)) {
			mAxisBehavior->StopPicking();
		}

		RenderSceneForActorPicking();

		auto rtDesc = mPickingRenderTarget->D3DResource()->GetDesc();

		uint8_t* mappedData = mPickingReadback->Map();
		uint32_t offset = ((rtDesc.Width * GHL::GetFormatStride(rtDesc.Format) + 0x0ff) & ~0x0ff) * mouseY;
		offset += (uint32_t)mouseX * GHL::GetFormatStride(rtDesc.Format);

		if (offset >= 0) {
			// 如果offset的值小于0可能会报段错误
			mappedData += offset;
			// 获取目标位置的像素值
			uint8_t pixels[3] = { mappedData[0], mappedData[1], mappedData[2] };

			std::optional<EAxisDirection> axisDirection = mAxisBehavior->IsPicking() ? mAxisBehavior->GetDirection() :
				(pixels[2] == 252 || pixels[2] == 253 || pixels[2] == 254) ? EAxisDirection(pixels[2] - 252) : std::optional<EAxisDirection>{};
			
			int32_t actorID = 0 << 24 | pixels[0] << 16 | pixels[1] << 8 | pixels[2];
			auto* underActor = mCurrentScene->FindActorByID(actorID);

			// 修改当前高亮的控制轴
			if (axisDirection.has_value()) {
				mAxisPassData.highlightedAxis = std::underlying_type<EAxisDirection>::type(*axisDirection);
			}
			else {
				mAxisPassData.highlightedAxis = 3u;
			}

			// Process Pressed
			if (CORESERVICE(Windows::InputManger).IsMouseButtonPressed(Windows::EMouseButton::MOUSE_LBUTTON)) {
				auto& inspectorPanel = CORESERVICE(UI::UIManger).GetCanvas()->GetPanel<App::Inspector>("Inspector");
				if (axisDirection.has_value()) {
					// 此时必须要有Focus Actor
					ASSERT_FORMAT(inspectorPanel.GetFocusActor() != nullptr, "No Focus Actor");
					mAxisBehavior->StartPicking(inspectorPanel.GetFocusActor(), mEditorTransform->worldPosition, *axisDirection, mAxisOperation);
				}
				else {
					if (underActor != nullptr) {
						inspectorPanel.FocusActor(underActor);
					}
					else {
						inspectorPanel.UnFocusActor();
					}
				}
			}

			// Process Picking
			if (mAxisBehavior->IsPicking()) {
				
			}
		}
	}

	void SceneView::RenderSceneForActorPicking() {
		// Create D3D Object First
		CreateD3DObjectForPicking();

		// Push Current Picking Frame
		mPickingFrameFence->IncrementExpectedValue();
		mPickingFrameTracker->PushCurrentFrame(mPickingFrameFence->ExpectedValue());

		// Prepare For Rendering
		auto* shaderManger = mRenderEngine->mShaderManger.get();
		auto* axisPickingShader  = shaderManger->GetShader<Renderer::GraphicsShader>("AxisPicking");
		// auto* actorPickingShader = shaderManger->GetShader<Renderer::GraphicsShader>("ActorPicking");
		auto& rtvHandle    = mPickingRTDescriptor.GetCpuHandle();
		FLOAT clearColor[] = { 0.0f, 0.0f, 0.0f, 0.0f };

		D3D12_VIEWPORT viewPort{};
		viewPort.TopLeftX = 0.0f;
		viewPort.TopLeftY = 0.0f;
		viewPort.Width = mAvailableSize.x;
		viewPort.Height = mAvailableSize.y;

		D3D12_RECT rect{};
		rect.left = 0;
		rect.top = 0;
		rect.right = mAvailableSize.x;
		rect.bottom = mAvailableSize.y;

		GHL::TransitionBarrier renderTargetTransiton(mPickingRenderTarget.get(), GHL::EResourceState::CopySource, GHL::EResourceState::RenderTarget);
		mPickingCommandList->D3DCommandList()->ResourceBarrier(1u, &renderTargetTransiton.D3DBarrier());

		mPickingCommandList->D3DCommandList()->RSSetViewports(1u, &viewPort);
		mPickingCommandList->D3DCommandList()->RSSetScissorRects(1u, &rect);
		mPickingCommandList->D3DCommandList()->ClearRenderTargetView(rtvHandle, clearColor, 1u, &rect);
		mPickingCommandList->D3DCommandList()->OMSetRenderTargets(1u, &rtvHandle, false, nullptr);
		mPickingCommandList->D3DCommandList()->SetGraphicsRootSignature(shaderManger->GetBaseD3DRootSignature());

		// Render Actors For Picking

		// 设置ActorPicking管线

		auto& inspectorPanel = CORESERVICE(UI::UIManger).GetCanvas()->GetPanel<App::Inspector>("Inspector");
		if (inspectorPanel.GetFocusActor() != nullptr) {
			// Render Axis For Picking
			auto* focusActor     = inspectorPanel.GetFocusActor();
			auto& actorTransform = focusActor->GetComponent<ECS::Transform>();

			// 更新AxisPassData
			mAxisPassData.modelMatrix = Math::Matrix4(actorTransform.worldPosition, actorTransform.worldRotation, Math::Vector3{ 1.0f, 1.0f, 1.0f }).Transpose();
			mAxisPassData.viewMatrix  = mEditorCamera->viewMatrix.Transpose();
			mAxisPassData.projMatrix  = mEditorCamera->projMatrix.Transpose();
			mAxisPassData.viewPos     = mEditorTransform->worldPosition;

			auto dynamicAllocation = mPickingLinearBufferAllocator->Allocate(sizeof(AxisPassData), 256u);
			memcpy(dynamicAllocation.cpuAddress, &mAxisPassData, sizeof(AxisPassData));

			// 设置AxisPicking管线
			mPickingCommandList->D3DCommandList()->SetPipelineState(axisPickingShader->GetD3DPipelineState());
			mPickingCommandList->D3DCommandList()->SetGraphicsRootConstantBufferView(1u, dynamicAllocation.gpuAddress);

			auto* axisMesh = CORESERVICE(Core::EditorAssetManger).GetMesh("Arrow_Translate");
			auto  axisVBView = axisMesh->GetVertexBuffer()->GetVBDescriptor();
			auto  axisIBView = axisMesh->GetIndexBuffer()->GetIBDescriptor();
			mPickingCommandList->D3DCommandList()->IASetVertexBuffers(0u, 1u, &axisVBView);
			mPickingCommandList->D3DCommandList()->IASetIndexBuffer(&axisIBView);
			mPickingCommandList->D3DCommandList()->IASetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			mPickingCommandList->D3DCommandList()->DrawIndexedInstanced(axisIBView.SizeInBytes / sizeof(uint32_t), 3u, 0u, 0u, 0u);
		}
		GHL::TransitionBarrier copySourceTransiton(mPickingRenderTarget.get(), GHL::EResourceState::RenderTarget, GHL::EResourceState::CopySource);
		mPickingCommandList->D3DCommandList()->ResourceBarrier(1u, &copySourceTransiton.D3DBarrier());
		
		// Read back
		auto srcDesc = mPickingRenderTarget->D3DResource()->GetDesc();
		// 纹理资源的一行是256字节的倍数
		uint32_t rowPitch = (srcDesc.Width * GHL::GetFormatStride(srcDesc.Format) + 0x0ff) & ~0x0ff;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{ 0,
			{ srcDesc.Format, (UINT)srcDesc.Width, srcDesc.Height, srcDesc.DepthOrArraySize, rowPitch } };

		D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(mPickingRenderTarget->D3DResource(), 0);
		D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(mPickingReadback->D3DResource(), layout);
		// 录制
		mPickingCommandList->D3DCommandList()->CopyTextureRegion(
			&dstLocation,
			0u, 0u, 0u,
			&srcLocation, nullptr
		);
		mPickingCommandList->Close();

		auto* graphicsQueue = mRenderEngine->mGraphicsQueue.get();
		graphicsQueue->ExecuteCommandList(mPickingCommandList->D3DCommandList());
		graphicsQueue->SignalFence(*mPickingFrameFence);

		mPickingFrameFence->Wait();
		
		mPickingFrameTracker->PopCompletedFrame(mPickingFrameFence->CompletedValue());

		mPickingCommandListAllocator->Reset();
		mPickingCommandList->Reset();
	}

	void SceneView::RegisterEditorRenderPass() {
		mRenderEngine->mEditorRenderPass += 
		[this](Renderer::CommandListWrap& commandList, Renderer::RenderContext& renderContext) {
			auto* axisRenderShader = renderContext.shaderManger->GetShader<Renderer::GraphicsShader>("AxisRender");
			auto  finalOutputRTV   = static_cast<Renderer::Texture*>(renderContext.resourceStorage->GetResourceByName("FinalOutput")->resource)->GetRTDescriptor()->GetCpuHandle();
		
			D3D12_VIEWPORT viewPort{};
			viewPort.TopLeftX = 0.0f;
			viewPort.TopLeftY = 0.0f;
			viewPort.Width = 979u;
			viewPort.Height = 635u;

			D3D12_RECT rect{};
			rect.left = 0;
			rect.top = 0;
			rect.right = 979;
			rect.bottom = 635;

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
		if (mPickingCommandListAllocator != nullptr) {
			return;
		}

		// Create D3D Object
		auto* device = mRenderEngine->mDevice.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();

		mPickingFrameTracker          = std::make_unique<Renderer::RingFrameTracker>(1u);
		mPickingLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mPickingFrameTracker.get());
		mPickingFrameFence            = std::make_unique<GHL::Fence>(device);

		mPickingCommandListAllocator = std::make_unique<GHL::CommandAllocator>(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
		mPickingCommandList = std::make_unique<GHL::CommandList>(device, mPickingCommandListAllocator->D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_DIRECT);

		Renderer::TextureDesc pickingRTDesc{};
		pickingRTDesc.width  = mAvailableSize.x;
		pickingRTDesc.height = mAvailableSize.y;
		pickingRTDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
		pickingRTDesc.initialState  = GHL::EResourceState::CopySource;
		pickingRTDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::CopySource;

		mPickingRenderTarget = std::make_unique<Renderer::Texture>(
			device,
			Renderer::ResourceFormat{ device, pickingRTDesc },
			descriptorAllocator,
			nullptr);

		mPickingRTDescriptorHeap = std::make_unique<GHL::DescriptorHeap>(device, D3D12_DESCRIPTOR_HEAP_TYPE_RTV, 1u);
		mPickingRTDescriptor     = mPickingRTDescriptorHeap->Allocate(0u);
		mPickingRenderTarget->BindRTDescriptor(mPickingRTDescriptor);

		Renderer::BufferDesc  pickingRBDesc{};
		pickingRBDesc.size = GetRequiredIntermediateSize(mPickingRenderTarget->D3DResource(), 0, 1);
		pickingRBDesc.usage = GHL::EResourceUsage::ReadBack;
		pickingRBDesc.initialState  = GHL::EResourceState::CopyDestination;
		pickingRBDesc.expectedState = GHL::EResourceState::CopyDestination;

		mPickingReadback = std::make_unique<Renderer::Buffer>(
			device,
			Renderer::ResourceFormat{ device, pickingRBDesc },
			descriptorAllocator,
			nullptr
		);
		mPickingReadback->Map();
	}
}