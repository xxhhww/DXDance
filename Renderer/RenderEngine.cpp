#include "Renderer/RenderEngine.h"
#include "Renderer/FormatConverter.h"

#include "ECS/Entity.h"
#include "ECS/CLight.h"
#include "ECS/CSky.h"

#include "GHL/Box.h"

#include "Math/Frustum.h"
#include "Math/Common.h"
#include "Math/HosekWilkieSkyModel.h"

namespace Renderer {
	RenderEngine::RenderEngine(HWND windowHandle, uint64_t width, uint64_t height, uint8_t numBackBuffers)
		: mWindowHandle(windowHandle)
		, mOutputWidth(width)
		, mOutputHeight(height)
		, mBackBufferStrategy((GHL::BackBufferStrategy)numBackBuffers)
		, mAdapterContainer(std::make_unique<GHL::AdapterContainer>())
		, mSelectedAdapter(mAdapterContainer->GetHighPerformanceAdapter())
		, mSelectedDisplay(&mSelectedAdapter->GetDisplay())
		, mDevice(std::make_unique<GHL::Device>(*mSelectedAdapter, false))
		, mUploaderEngine(std::make_unique<UploaderEngine>(mDevice.get()))
		, mGraphicsQueue(std::make_unique<GHL::GraphicsQueue>(mDevice.get()))
		, mComputeQueue(std::make_unique<GHL::ComputeQueue>(mDevice.get()))
		, mCopyQueue(std::make_unique<GHL::CopyQueue>(mDevice.get()))
		, mRenderFrameFence(std::make_unique<GHL::Fence>(mDevice.get()))
		, mFrameTracker(std::make_unique<RingFrameTracker>(numBackBuffers))
		, mHeapAllocator(std::make_unique<BuddyHeapAllocator>(mDevice.get(), mFrameTracker.get()))
		, mCommandListAllocator(std::make_unique<PoolCommandListAllocator>(mDevice.get(), mFrameTracker.get()))
		, mDescriptorAllocator(std::make_unique<PoolDescriptorAllocator>(mDevice.get(), mFrameTracker.get(), std::vector<uint64_t>{1024, 128, 128, 128}))
		, mSharedMemAllocator(std::make_unique<LinearBufferAllocator>(mDevice.get(), mFrameTracker.get()))
		, mGPUProfiler(std::make_unique<GPUProfiler>(mDevice.get(), mFrameTracker.get()))
		, mShaderManger(std::make_unique<ShaderManger>(mDevice.get()))
		, mCommandSignatureManger(std::make_unique<CommandSignatureManger>(mDevice.get()))
		, mResourceStateTracker(std::make_unique<ResourceStateTracker>())
		, mStreamTextureManger(std::make_unique<StreamTextureManger>(
			mDevice.get(),
			mCopyQueue.get(),
			mDescriptorAllocator.get(),
			mHeapAllocator.get(),
			mFrameTracker.get(),
			mUploaderEngine->GetDSFactory(),
			mUploaderEngine->GetFileCopyQueue(),
			mUploaderEngine->GetMemoryCopyQueue()))
		, mRenderGraph(std::make_unique<RenderGraph>(
			mSelectedDisplay,
			mDevice.get(),
			mFrameTracker.get(),
			mDescriptorAllocator.get(),
			mCommandListAllocator.get(),
			mGraphicsQueue.get(),
			mComputeQueue.get(),
			mCopyQueue.get(),
			mResourceStateTracker.get(),
			mShaderManger.get(),
			mCommandSignatureManger.get(),
			mSharedMemAllocator.get(),
			mStreamTextureManger.get()))
		, mPipelineResourceStorage(mRenderGraph->GetPipelineResourceStorage()) 
		, mOfflineFence(std::make_unique<GHL::Fence>(mDevice.get())) {

		mGraphicsQueue->SetDebugName("GraphicsQueue");
		mComputeQueue->SetDebugName("ComputeQueue");
		mCopyQueue->SetDebugName("CopyQueue");

		if (windowHandle != nullptr) {
			mSwapChain = std::make_unique<GHL::SwapChain>(&mSelectedAdapter->GetDisplay(), mGraphicsQueue->D3DCommandQueue(), windowHandle, mBackBufferStrategy, width, height);
			for (uint32_t i = 0; i < numBackBuffers; i++) {
				mBackBuffers.emplace_back(std::make_unique<Texture>(mDevice.get(), mSwapChain->D3DBackBuffer(i), mDescriptorAllocator.get()));
				mResourceStateTracker->StartTracking(mBackBuffers.at(i).get());
			}
			mShaderManger->CreateGraphicsShader("OutputBackBuffer",
				[&](GraphicsStateProxy& proxy) {
					proxy.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
					proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/OutputBackBuffer/OutputBackBuffer.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
				});

			std::vector<Vertex> vertices;
			vertices.resize(6u);
			vertices[0].position = Math::Vector3{ -1.0f, 1.0f, 0.0f  };
			vertices[1].position = Math::Vector3{ 1.0f, -1.0f, 0.0f  };
			vertices[2].position = Math::Vector3{ -1.0f, -1.0f, 0.0f };
			vertices[3].position = Math::Vector3{ -1.0f, 1.0f, 0.0f  };
			vertices[4].position = Math::Vector3{ 1.0f, 1.0f, 0.0f   };
			vertices[5].position = Math::Vector3{ 1.0f, -1.0f, 0.0f  };
			vertices[0].uv = Math::Vector2{ 0.0f, 0.0f };
			vertices[1].uv = Math::Vector2{ 1.0f, 1.0f };
			vertices[2].uv = Math::Vector2{ 0.0f, 1.0f };
			vertices[3].uv = Math::Vector2{ 0.0f, 0.0f };
			vertices[4].uv = Math::Vector2{ 1.0f, 0.0f };
			vertices[5].uv = Math::Vector2{ 1.0f, 1.0f };

			std::vector<uint32_t> indices = { 0, 1, 2, 3 };

			Renderer::BufferDesc vbDesc{};
			vbDesc.stride = sizeof(Renderer::Vertex);
			vbDesc.size = vbDesc.stride * vertices.size();
			vbDesc.usage = GHL::EResourceUsage::Default;

			Renderer::BufferDesc ibDesc{};
			ibDesc.stride = sizeof(uint32_t);
			ibDesc.size = ibDesc.stride * indices.size();
			ibDesc.usage = GHL::EResourceUsage::Default;

			mOutputQuadMesh = std::make_unique<Renderer::Mesh>(
				mDevice.get(),
				ResourceFormat{ mDevice.get(), vbDesc },
				ResourceFormat{ mDevice.get(), ibDesc },
				nullptr, nullptr
				);

			indices.clear();
			mOutputQuadMesh->LoadDataFromMemory(mUploaderEngine->GetMemoryCopyQueue(),
				mUploaderEngine->GetCopyFence(), vertices, indices);
		}

		// 创建FinalOutput纹理
		{
			TextureDesc _FinalOutputDesc{};
			_FinalOutputDesc.width = width;
			_FinalOutputDesc.height = height;
			_FinalOutputDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_FinalOutputDesc.expectedState |= (GHL::EResourceState::PixelShaderAccess | GHL::EResourceState::RenderTarget | GHL::EResourceState::UnorderedAccess);
			_FinalOutputDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
			mFinalOutput = std::make_unique<Texture>(mDevice.get(), ResourceFormat{ mDevice.get(), _FinalOutputDesc }, mDescriptorAllocator.get(), nullptr);
			mFinalOutput->SetDebugName("FinalOutput");
			mFinalOutputID = mRenderGraph->ImportResource("FinalOutput", mFinalOutput.get());

			mResourceStateTracker->StartTracking(mFinalOutput.get());
		}

		// 创建BlueNoise3D纹理
		{
			DirectX::ScratchImage baseImage;
			HRASSERT(DirectX::LoadFromDDSFile(
				L"E:/MyProject/DXDance/Resources/Textures/BlueNoise3D.dds",
				DirectX::DDS_FLAGS_NONE,
				nullptr,
				baseImage
			));

			Renderer::TextureDesc _BlueNoise3DMapDesc = FormatConverter::GetTextureDesc(baseImage.GetMetadata());
			_BlueNoise3DMapDesc.expectedState = GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::PixelShaderAccess;
			mBlueNoise3DMap = std::make_unique<Texture>(
				mDevice.get(),
				ResourceFormat{ mDevice.get(), _BlueNoise3DMapDesc },
				mDescriptorAllocator.get(),
				nullptr
				);

			// 获取纹理GPU存储信息
			uint32_t subresourceCount = mBlueNoise3DMap->GetResourceFormat().SubresourceCount();
			std::vector<D3D12_PLACED_SUBRESOURCE_FOOTPRINT> placedLayouts(subresourceCount);
			std::vector<uint32_t> numRows(subresourceCount);
			std::vector<uint64_t> rowSizesInBytes(subresourceCount);
			uint64_t requiredSize = 0u;
			auto d3dResDesc = mBlueNoise3DMap->GetResourceFormat().D3DResourceDesc();
			mDevice->D3DDevice()->GetCopyableFootprints(&d3dResDesc, 0u, 1u, 0u,
				placedLayouts.data(), numRows.data(), rowSizesInBytes.data(), &requiredSize);

			// 上传数据至显存
			uint8_t* temp = new uint8_t[requiredSize];
			auto* copyDsQueue = mUploaderEngine->GetMemoryCopyQueue();
			auto* copyFence = mUploaderEngine->GetCopyFence();
			for (uint32_t subresourceIndex = 0u; subresourceIndex < subresourceCount; subresourceIndex++) {
				for (uint32_t sliceIndex = 0u; sliceIndex < d3dResDesc.DepthOrArraySize; sliceIndex++) {
					auto* image = baseImage.GetImage(0u, 0u, sliceIndex);

					for (uint32_t rowIndex = 0u; rowIndex < numRows[subresourceIndex]; rowIndex++) {
						uint32_t realByteOffset = rowIndex * placedLayouts.at(subresourceIndex).Footprint.RowPitch + sliceIndex * placedLayouts.at(subresourceIndex).Footprint.RowPitch * numRows[subresourceIndex];
						uint32_t fakeByteOffset = rowIndex * image->rowPitch;
						memcpy(temp + realByteOffset, image->pixels + fakeByteOffset, image->rowPitch);
					}
				}

				DSTORAGE_REQUEST request = {};
				request.Options.CompressionFormat = DSTORAGE_COMPRESSION_FORMAT_NONE;
				request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
				request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
				request.Source.Memory.Source = temp;
				request.Source.Memory.Size = requiredSize;
				request.Destination.Texture.Resource = mBlueNoise3DMap->D3DResource();
				request.Destination.Texture.Region = GHL::Box {
					0u, placedLayouts.at(subresourceIndex).Footprint.Width,
					0u, placedLayouts.at(subresourceIndex).Footprint.Height,
					0u, placedLayouts.at(subresourceIndex).Footprint.Depth
				}.D3DBox();
				request.Destination.Texture.SubresourceIndex = subresourceIndex;
				request.UncompressedSize = requiredSize;

				copyDsQueue->EnqueueRequest(&request);
			}
			copyFence->IncrementExpectedValue();
			copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
			copyDsQueue->Submit();
			copyFence->Wait();

			baseImage.Release();

			mBlueNoise3DMap->SetDebugName("BlueNoise3DMap");
			mBlueNoise3DMapID = mRenderGraph->ImportResource("BlueNoise3DMap", mBlueNoise3DMap.get());

			mResourceStateTracker->StartTracking(mBlueNoise3DMap.get());
		}

		// 初始化RenderPass
		{
			mTerrainPass.InitializePass(this);
		}

		// 添加RenderPass并构建RenderGraph
		{
			mGBufferPass.AddPass(*mRenderGraph);
			mTerrainPass.AddPass(*mRenderGraph);
			mRngSeedGenerationPass.AddPass(*mRenderGraph);
			mSkyGenerationPass.AddPass(*mRenderGraph);
			mDeferredLightPass.AddPass(*mRenderGraph);
			mToneMappingPass.AddPass(*mRenderGraph);
			mFinalBarrierPass.AddPass(*mRenderGraph);

			mRenderGraph->Build();
		}

		// Other
		mStreamTextureManger->Request("E:/MyProject/DXDance/Renderer/media/4ktiles.xet");
	}

	RenderEngine::~RenderEngine() {
	}

	void RenderEngine::Resize(uint64_t width, uint64_t height) {

	}

	void RenderEngine::Update(float dt, const ECS::Camera& editorCamera, const ECS::Transform& cameraTransform) {
		// 清理工作
		mPipelineResourceStorage->rootLightDataPerFrame.clear();
		
		// 更新工作
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.position = cameraTransform.worldPosition;
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.view = editorCamera.viewMatrix.Transpose();
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.projection = editorCamera.projMatrix.Transpose();
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.viewProjection = (editorCamera.viewMatrix * editorCamera.projMatrix).Transpose();
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.inverseView = editorCamera.viewMatrix.Inverse().Transpose();
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.inverseProjection = editorCamera.projMatrix.Inverse().Transpose();
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.farPlane = editorCamera.frustum.farZ;
		mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.exposureValue100 = editorCamera.GetExposureValue100();

		ECS::Entity::Foreach([&](ECS::Camera& camera, ECS::Transform& transform) {
			if (camera.mainCamera == true && camera.cameraType == ECS::CameraType::RenderCamera) {
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.position = transform.worldPosition;
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.view = camera.viewMatrix.Transpose();
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.projection = camera.projMatrix.Transpose();
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.viewProjection = (camera.viewMatrix * camera.projMatrix).Transpose();
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.inverseView = camera.viewMatrix.Inverse().Transpose();
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.inverseProjection = camera.projMatrix.Inverse().Transpose();
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.farPlane = camera.frustum.farZ;
				mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.exposureValue100 = camera.GetExposureValue100();
				Math::Frustum::BuildFrustumPlanes(
					camera.viewMatrix * camera.projMatrix,
					mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.planes
				);
			}
		});

		UpdateSky();
		UpdateLights();
	}

	void RenderEngine::UpdateSky() {
		ECS::Entity::ForeachInCurrentThread([&](ECS::Transform& transform, ECS::Sky& sky) {

			// https://www.gamedev.net/forums/topic/671214-simple-solar-radiance-calculation/

			const Math::Vector3 sunDirection = transform.GetDirection();
			
			float thetaS = std::acos(1.0f - sunDirection.y);
			float elevation = DirectX::XM_PIDIV2 - thetaS;
			float turbidity = sky.turbidity;

			uint32_t totalSampleCount = sky.skySpectrum.GetSamples().size();

			// Vertical sample angle. For one ray it's just equal to elevation.
			float theta = elevation;

			// Angle between the sun direction and sample direction.
			// Since we have only one sample for simplicity, the angle is 0.
			float gamma = 0.0;

			// We compute spectrum for the middle ray at the center of the Sun's disk.
			// For simplicity, we ignore limb darkening.
			for (uint64_t i = 0; i < totalSampleCount; ++i) {
				ArHosekSkyModelState* skyState = arhosekskymodelstate_alloc_init(elevation, turbidity, sky.groundAlbedoSpectrum[i]);
				float wavelength = Math::Mix(sky.skySpectrum.LowestWavelength(), sky.skySpectrum.HighestWavelength(), (float)i / float(totalSampleCount));
				sky.skySpectrum[i] = float(arhosekskymodel_solar_radiance(skyState, theta, gamma, wavelength));
				arhosekskymodelstate_free(skyState);
				skyState = nullptr;
			}

			if (sky.skyModelStateR)
				arhosekskymodelstate_free(sky.skyModelStateR);

			if (sky.skyModelStateG)
				arhosekskymodelstate_free(sky.skyModelStateG);

			if (sky.skyModelStateB)
				arhosekskymodelstate_free(sky.skyModelStateB);

			Math::Vector3 sunLuminance = sky.skySpectrum.ToRGB();
			Math::Vector3 sunIlluminance = sunLuminance * ECS::SunSolidAngle; // Dividing by 1 / PDF

			// Found it on internet, without it the illuminance is too small.
			// Account for luminous efficacy, coordinate system scaling (100, wtf???)
			float multiplier = 1.0f;
			sky.sunIlluminance = sunIlluminance * multiplier;
			sky.sunLuminance = sunLuminance * multiplier;

			sky.skyModelStateR = arhosek_rgb_skymodelstate_alloc_init(turbidity, sky.groundAlbedo.x, elevation);
			sky.skyModelStateG = arhosek_rgb_skymodelstate_alloc_init(turbidity, sky.groundAlbedo.y, elevation);
			sky.skyModelStateB = arhosek_rgb_skymodelstate_alloc_init(turbidity, sky.groundAlbedo.z, elevation);
			
			// Sun Light
			auto& gpuLightData = mPipelineResourceStorage->rootLightDataPerFrame.emplace_back();
			gpuLightData.position = Math::Vector4{ sunDirection, ECS::SunDiskArea };
			gpuLightData.color = sky.sunIlluminance;
			gpuLightData.type = std::underlying_type<ECS::LightType>::type(ECS::LightType::Sun);
		});
	}

	void RenderEngine::UpdateLights() {
		// 在当前线程内部逐一遍历实体集合
		ECS::Entity::ForeachInCurrentThread([&](ECS::Transform& transform, ECS::Light& light) {
			auto& gpuLightData = mPipelineResourceStorage->rootLightDataPerFrame.emplace_back();
			gpuLightData.position = transform.worldPosition;
			gpuLightData.color = light.color;
			gpuLightData.type = std::underlying_type<ECS::LightType>::type(light.lightType);
		});

		mPipelineResourceStorage->rootConstantsPerFrame.lightSize =
			mPipelineResourceStorage->rootLightDataPerFrame.size();
	}

	void RenderEngine::Render() {
		// 压入新的渲染帧
		mRenderFrameFence->IncrementExpectedValue();
		mFrameTracker->PushCurrentFrame(mRenderFrameFence->ExpectedValue());

		auto rootDataAllocation = LinearAllocation{};
		// RootConstantsDataPerFrame
		rootDataAllocation = mSharedMemAllocator->Allocate(sizeof(RootConstantsPerFrame));
		memcpy(rootDataAllocation.cpuAddress, &mPipelineResourceStorage->rootConstantsPerFrame, sizeof(RootConstantsPerFrame));
		mPipelineResourceStorage->rootConstantsPerFrameAddress = rootDataAllocation.gpuAddress;

		// RootGPULightDataPerFrame
		size_t lightDataByteSize = 
			sizeof(GPULight) * mPipelineResourceStorage->rootLightDataPerFrame.size();
		rootDataAllocation = mSharedMemAllocator->Allocate(lightDataByteSize);
		memcpy(rootDataAllocation.cpuAddress, mPipelineResourceStorage->rootLightDataPerFrame.data(), lightDataByteSize);
		mPipelineResourceStorage->rootLightDataPerFrameAddress = rootDataAllocation.gpuAddress;

		mRenderGraph->Execute();

		RenderContext renderContext{
			mSelectedDisplay,
			mDevice.get(),
			mShaderManger.get(),
			mCommandSignatureManger.get(),
			mSharedMemAllocator.get(),
			mRenderGraph->GetPipelineResourceStorage(),
			mResourceStateTracker.get(),
			mStreamTextureManger.get(),
			mFrameTracker.get()
		};
		{
			auto commandList = mCommandListAllocator->AllocateGraphicsCommandList();
			CommandBuffer commandBuffer{ commandList.Get(), &renderContext };

			// 录制Editor Render Pass命令
			commandBuffer.PIXBeginEvent("EditorRenderPass");
			mEditorRenderPass.Invoke(commandBuffer, renderContext);
			commandBuffer.PIXEndEvent();

			// 将FinalOutput转换为PixelShaderAccess供GPU读取
			commandBuffer.PIXBeginEvent("FinalOutputBarrierPass");
			auto barrierBatch = mResourceStateTracker->TransitionImmediately(mFinalOutput.get(), GHL::EResourceState::PixelShaderAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.PIXEndEvent();

			commandList->Close();
			mGraphicsQueue->ExecuteCommandList(commandList->D3DCommandList());
		}

		if (mWindowHandle != nullptr) {
			{
				uint64_t srvIndex = mFinalOutput->GetSRDescriptor()->GetHeapIndex();
				auto commandList = mCommandListAllocator->AllocateGraphicsCommandList();
				auto* descriptorHeap = mDescriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
				commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
				CommandBuffer commandBuffer{ commandList.Get(), &renderContext };
				commandBuffer.PIXBeginEvent("OutputBackBufferPass");

				auto  currentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
				auto* currentBackBuffer = mBackBuffers.at(currentBackBufferIndex).get();
				auto barrierBatch = commandBuffer.TransitionImmediately(currentBackBuffer, GHL::EResourceState::RenderTarget);
				commandBuffer.FlushResourceBarrier(barrierBatch);
				commandBuffer.ClearRenderTarget(currentBackBuffer);
				commandBuffer.SetRenderTarget(currentBackBuffer);
				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, 979u, 635u });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, 979u, 635u });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("OutputBackBuffer");
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.SetVertexBuffer(0u, mOutputQuadMesh->GetVertexBuffer());
				commandBuffer.DrawInstanced(mOutputQuadMesh->GetVertexCount(), 1u, 0u, 0u);
				barrierBatch = commandBuffer.TransitionImmediately(currentBackBuffer, GHL::EResourceState::Present);
				commandBuffer.FlushResourceBarrier(barrierBatch);

				commandBuffer.PIXEndEvent();
				commandList->Close();
				mGraphicsQueue->ExecuteCommandList(commandList->D3DCommandList());
				
				mSwapChain->Present(1u);
			}
		}

		// 压入渲染命令完成后的围栏
		mGraphicsQueue->SignalFence(*mRenderFrameFence.get());

		// TODO 除了等待GPU任务的完成外，渲染主线程还需要等待ProcessThread线程的任务完成情况
		if (mFrameTracker->GetUsedSize() == std::underlying_type<GHL::BackBufferStrategy>::type(mBackBufferStrategy)) {
			HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

			UINT64 valueToWaitFor = mRenderFrameFence->ExpectedValue() - (std::underlying_type<GHL::BackBufferStrategy>::type(mBackBufferStrategy) - 1u);
			mRenderFrameFence->SetCompletionEvent(valueToWaitFor, eventHandle);

			// Wait until the GPU hits current fence event is fired.
			WaitForSingleObject(eventHandle, INFINITE);
			CloseHandle(eventHandle);
		}

		// 检测并处理渲染帧是否完成
		mFrameTracker->PopCompletedFrame(mRenderFrameFence->CompletedValue());
	}

	void RenderEngine::DoOfflineTask() {
		// 压入新的渲染帧(执行渲染操作是为了能在PIX中对离线任务进行调试)
		mRenderFrameFence->IncrementExpectedValue();
		mFrameTracker->PushCurrentFrame(mRenderFrameFence->ExpectedValue());

		RenderContext renderContext{
			mSelectedDisplay,
			mDevice.get(),
			mShaderManger.get(),
			mCommandSignatureManger.get(),
			mSharedMemAllocator.get(),
			mRenderGraph->GetPipelineResourceStorage(),
			mResourceStateTracker.get(),
			mStreamTextureManger.get(),
			mFrameTracker.get()
		};

		if (mOfflineTaskPass.GetListenerCount() != 0u) {
			// 标识新的任务
			mOfflineFence->IncrementExpectedValue();
			{
				auto commandList = mCommandListAllocator->AllocateComputeCommandList();
				auto* descriptorHeap = mDescriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
				commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
				CommandBuffer commandBuffer{ commandList.Get(), &renderContext };

				// 录制Offline Task Pass命令
				mOfflineTaskPass.Invoke(commandBuffer, renderContext);

				// 执行离线任务
				commandList->Close();
				mComputeQueue->ExecuteCommandList(commandList->D3DCommandList());

				// 设置完成后的达到的值
				mComputeQueue->SignalFence(*mOfflineFence.get());
			}
			// 同步等待离线任务的完成
			mOfflineFence->Wait();

			mOfflineCompletedCallback.Invoke();
		}

		ASSERT_FORMAT(mWindowHandle != nullptr, "Offline Task Configuration Error: Window Handle Missing");
		{
			uint64_t srvIndex = mFinalOutput->GetSRDescriptor()->GetHeapIndex();
			auto commandList = mCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = mDescriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), &renderContext };

			auto  currentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
			auto* currentBackBuffer = mBackBuffers.at(currentBackBufferIndex).get();
			auto barrierBatch = commandBuffer.TransitionImmediately(currentBackBuffer, GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.ClearRenderTarget(currentBackBuffer);
			commandBuffer.SetRenderTarget(currentBackBuffer);
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, 979u, 635u });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, 979u, 635u });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState("OutputBackBuffer");
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.SetVertexBuffer(0u, mOutputQuadMesh->GetVertexBuffer());
			commandBuffer.DrawInstanced(mOutputQuadMesh->GetVertexCount(), 1u, 0u, 0u);
			barrierBatch = commandBuffer.TransitionImmediately(currentBackBuffer, GHL::EResourceState::Present);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandList->Close();
			mGraphicsQueue->ExecuteCommandList(commandList->D3DCommandList());

			mSwapChain->Present(1u);

			// 设置完成后的达到的值
			mGraphicsQueue->SignalFence(*mRenderFrameFence.get());
		}

		// 同步等待渲染帧的完成
		mRenderFrameFence->Wait();
		mFrameTracker->PopCompletedFrame(mRenderFrameFence->CompletedValue());
	}

	void RenderEngine::BindFinalOuputSRV(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle) {
		mFinalOutput->BindSRDescriptor(cpuHandle);
	}
}