#include "Renderer/RenderEngine.h"
#include "Renderer/FixedTextureHelper.h"

#include "Renderer/TerrainSystem.h"

#include "Renderer/TerrainTextureAtlas.h"

#include "ECS/Entity.h"
#include "ECS/CLight.h"
#include "ECS/CSky.h"
#include "ECS/CTransform.h"
#include "ECS/CMeshRenderer.h"

#include "GHL/Box.h"

#include "Math/Frustum.h"
#include "Math/Common.h"
#include "Math/HosekWilkieSkyModel.h"
#include "Math/Jitter.h"

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
		, mDStorageFactory(std::make_unique<GHL::DirectStorageFactory>())
		, mDStorageFileQueue(std::make_unique<GHL::DirectStorageQueue>(mDevice.get(), mDStorageFactory.get(), DSTORAGE_REQUEST_SOURCE_FILE))
		, mDStorageMemQueue(std::make_unique<GHL::DirectStorageQueue>(mDevice.get(), mDStorageFactory.get(), DSTORAGE_REQUEST_SOURCE_MEMORY))
		, mDStorageFence(std::make_unique<GHL::Fence>(mDevice.get()))
		, mGraphicsQueue(std::make_unique<GHL::GraphicsQueue>(mDevice.get()))
		, mComputeQueue(std::make_unique<GHL::ComputeQueue>(mDevice.get()))
		, mCopyQueue(std::make_unique<GHL::CopyQueue>(mDevice.get()))
		, mRenderFrameFence(std::make_unique<GHL::Fence>(mDevice.get()))
		, mFrameTracker(std::make_unique<RingFrameTracker>(numBackBuffers))
		, mHeapAllocator(std::make_unique<BuddyHeapAllocator>(mDevice.get(), mFrameTracker.get()))
		, mCommandListAllocator(std::make_unique<PoolCommandListAllocator>(mDevice.get(), mFrameTracker.get()))
		, mDescriptorAllocator(std::make_unique<PoolDescriptorAllocator>(mDevice.get(), mFrameTracker.get(), std::vector<uint64_t>{1024, 128, 128, 128}))
		, mResourceAllocator(std::make_unique<ResourceAllocator>(mFrameTracker.get()))
		, mSharedMemAllocator(std::make_unique<LinearBufferAllocator>(mDevice.get(), mFrameTracker.get()))
		, mGPUProfiler(std::make_unique<GPUProfiler>(mDevice.get(), mFrameTracker.get()))
		, mShaderManger(std::make_unique<ShaderManger>(mDevice.get()))
		, mCommandSignatureManger(std::make_unique<CommandSignatureManger>(mDevice.get()))
		, mResourceStateTracker(std::make_unique<ResourceStateTracker>())
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
			nullptr))
		, mPipelineResourceStorage(mRenderGraph->GetPipelineResourceStorage())
		, mOfflineFence(std::make_unique<GHL::Fence>(mDevice.get())) {

		mGraphicsQueue->SetDebugName("GraphicsQueue");
		mComputeQueue->SetDebugName("ComputeQueue");
		mCopyQueue->SetDebugName("CopyQueue");

		{
			/*
			uint32_t subresourceCount = 2048;

			D3D12_RESOURCE_DESC d3dDesc{};

			d3dDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			d3dDesc.Format = DXGI_FORMAT_R16_UINT;
			d3dDesc.MipLevels = 1u;
			d3dDesc.DepthOrArraySize = 2048;
			d3dDesc.Width = 256;
			d3dDesc.Height = 128;
			d3dDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
			d3dDesc.SampleDesc.Count = 1;
			D3D12_TILED_RESOURCES_TIER_2;
			Microsoft::WRL::ComPtr<ID3D12Resource> d3dResource;
			mDevice->D3DDevice()->CreateReservedResource(&d3dDesc, D3D12_RESOURCE_STATE_COMMON, nullptr, IID_PPV_ARGS(&d3dResource));

			D3D12_PACKED_MIP_INFO mPackedMipInfo; // last n mips may be packed into a single tile
			D3D12_TILE_SHAPE mTileShape;          // e.g. a 64K tile may contain 128x128 texels @ 4B/pixel
			UINT mNumTilesTotal;
			std::vector<D3D12_SUBRESOURCE_TILING> mTiling;
			uint8_t mNumStandardMips{ 0u }; // Start From 1u
			mTiling.resize(subresourceCount);

			mDevice->D3DDevice()->GetResourceTiling(d3dResource.Get(), &mNumTilesTotal, &mPackedMipInfo, &mTileShape, &subresourceCount, 0, &mTiling[0]);
		
			int32_t i = 32;
			*/
		}

		if (mWindowHandle != nullptr) {
			mSwapChain = std::make_unique<GHL::SwapChain>(&mSelectedAdapter->GetDisplay(), mGraphicsQueue->D3DCommandQueue(), mWindowHandle, mBackBufferStrategy, width, height);
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

		// 创建BlueNoise3D纹理
		{
			mBlueNoise3DMap = FixedTextureHelper::LoadFromFile(
				mDevice.get(), mDescriptorAllocator.get(), mResourceAllocator.get(),
				mUploaderEngine->GetMemoryCopyQueue(), mUploaderEngine->GetCopyFence(),
				"E:/MyProject/DXDance/Resources/Textures/Common/BlueNoise3D.dds");
			mBlueNoise3DMapID = mRenderGraph->ImportResource("BlueNoise3DMap", mBlueNoise3DMap.Get());
			mResourceStateTracker->StartTracking(mBlueNoise3DMap.Get());

			mBlueNoise2DMap = FixedTextureHelper::LoadFromFile(
				mDevice.get(), mDescriptorAllocator.get(), mResourceAllocator.get(),
				mUploaderEngine->GetMemoryCopyQueue(), mUploaderEngine->GetCopyFence(),
				"E:/MyProject/DXDance/Resources/Textures/Common/BlueNoise2D.dds");
			mBlueNoise2DMapID = mRenderGraph->ImportResource("BlueNoise2DMap", mBlueNoise2DMap.Get());
			mResourceStateTracker->StartTracking(mBlueNoise2DMap.Get());
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

		// 创建ItemDataBuffer ItemIndirectDrawIndexedBuffer
		{
			BufferDesc _DeferredItemDataBufferDesc{};
			_DeferredItemDataBufferDesc.stride = sizeof(GpuItemData);
			_DeferredItemDataBufferDesc.size = _DeferredItemDataBufferDesc.stride * smMaxItemNums;
			_DeferredItemDataBufferDesc.usage = GHL::EResourceUsage::Default;
			_DeferredItemDataBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_DeferredItemDataBufferDesc.initialState = GHL::EResourceState::Common;
			_DeferredItemDataBufferDesc.expectedState = GHL::EResourceState::CopySource | GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mDeferredItemDataBuffer = mResourceAllocator->Allocate(mDevice.get(), _DeferredItemDataBufferDesc, mDescriptorAllocator.get(), nullptr);
			mDeferredItemDataBuffer->SetDebugName("OpaqueItemDataBuffer");
			mDeferredItemDataBufferID = mRenderGraph->ImportResource("OpaqueItemDataBuffer", mDeferredItemDataBuffer);
			mResourceStateTracker->StartTracking(mDeferredItemDataBuffer);
			mResourceStateTracker->StartTracking(mDeferredItemDataBuffer->GetCounterBuffer());

			BufferDesc _DeferredItemIndirectDrawIndexedDataBufferDesc{};
			_DeferredItemIndirectDrawIndexedDataBufferDesc.stride = sizeof(GpuItemIndirectDrawIndexedData);
			_DeferredItemIndirectDrawIndexedDataBufferDesc.size = _DeferredItemIndirectDrawIndexedDataBufferDesc.stride * smMaxItemNums;
			_DeferredItemIndirectDrawIndexedDataBufferDesc.usage = GHL::EResourceUsage::Default;
			_DeferredItemIndirectDrawIndexedDataBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_DeferredItemIndirectDrawIndexedDataBufferDesc.initialState = GHL::EResourceState::Common;
			_DeferredItemIndirectDrawIndexedDataBufferDesc.expectedState = GHL::EResourceState::CopySource | GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mDeferredItemIndirectDrawIndexedDataBuffer = mResourceAllocator->Allocate(mDevice.get(), _DeferredItemIndirectDrawIndexedDataBufferDesc, mDescriptorAllocator.get(), nullptr);
			mDeferredItemIndirectDrawIndexedDataBuffer->SetDebugName("OpaqueItemIndirectDrawIndexedDataBuffer");
			mDeferredItemIndirectDrawIndexedDataBufferID = mRenderGraph->ImportResource("OpaqueItemIndirectDrawIndexedDataBuffer", mDeferredItemIndirectDrawIndexedDataBuffer);
			mResourceStateTracker->StartTracking(mDeferredItemIndirectDrawIndexedDataBuffer);
			mResourceStateTracker->StartTracking(mDeferredItemIndirectDrawIndexedDataBuffer->GetCounterBuffer());
		}

		// 初始化一些系统
		{
			mTerrainRenderer = std::make_unique<TerrainRenderer>(this);
			mTerrainSystem = std::make_unique<TerrainSystem>(this);
			// mVegetationSystem = std::make_unique<VegetationSystem>(this);
			mDetailObjectSystem = std::make_unique<DetailObjectSystem>(this);
		}

		// 初始化RenderPass
		{
			mTerrainRenderer->Initialize();
			// mTerrainSystem->Initialize(this);
			// mVegetationSystem->Initialize(this);
			// mDetailObjectSystem->Initialize(this);
			// mGrassPass.InitializePass(this);
			// mFoliagePass.InitializePass(this);
			mAtmospherePass.Initialize(this);
			// mOceanPass.InitializePass(this);
			// mVolumetricCloudsPass.InitializePass(this);
			mCascadeShadowPass.Initialize(this);
		}

		// 添加RenderPass并构建RenderGraph
		{
			/*
			mCascadeShadowPass.AddPass(this);
			mOpaquePass.AddPass(*mRenderGraph);
			mRngSeedGenerationPass.AddPass(*mRenderGraph);
			mTerrainSystem->AddPass(this);
			// mVegetationSystem->AddPass(this);
			// mDetailObjectSystem->AddPass(this);
			// mTerrainPass.AddForwardPlusPass(*mRenderGraph);
			// mGrassPass.AddPass(*mRenderGraph);
			mDeferredLightPass.AddPass(*mRenderGraph);
			mAtmospherePass.AddPass(*mRenderGraph);
			// mOceanPass.AddPass(*mRenderGraph);
			// mSkyPass.AddPass(*mRenderGraph);
			mTAAPass.AddPass(*mRenderGraph);
			mToneMappingPass.AddPass(*mRenderGraph);
			*/

			mCascadeShadowPass.AddPass(this);
			mOpaquePass.AddPass(*mRenderGraph);
			mRngSeedGenerationPass.AddPass(*mRenderGraph);
			mTerrainRenderer->AddPass();
			mDeferredLightPass.AddPass(*mRenderGraph);
			mAtmospherePass.AddPass(*mRenderGraph);
			mTAAPass.AddPass(*mRenderGraph);
			mToneMappingPass.AddPass(*mRenderGraph);
			mFinalBarrierPass.AddPass(*mRenderGraph);
			
			/*
			mGBufferPass.AddPass(*mRenderGraph);
			mTerrainPass.AddPass(*mRenderGraph);
			mFoliagePass.AddPass(*mRenderGraph);
			mRngSeedGenerationPass.AddPass(*mRenderGraph);
			mSkyGenerationPass.AddPass(*mRenderGraph);
			mDeferredLightPass.AddPass(*mRenderGraph);
			mVolumetricCloudsPass.AddPass(*mRenderGraph);
			mTAAPass.AddPass(*mRenderGraph);
			mToneMappingPass.AddPass(*mRenderGraph);
			mFinalBarrierPass.AddPass(*mRenderGraph);
			*/
			mRenderGraph->Build();
		}

		// Other
		// mStreamTextureManger->Request("E:/MyProject/DXDance/Renderer/media/4ktiles.xet");
	}

	RenderEngine::~RenderEngine() {
	}

	void RenderEngine::Resize(uint64_t width, uint64_t height) {

	}

	void RenderEngine::Update(float deltaTime, float totalTime) {
		// 清理工作
		mPipelineResourceStorage->rootLightDataPerFrame.clear();
		mPipelineResourceStorage->rootItemDataPerFrame.resize(smMaxItemNums, GpuItemData{});
		mPipelineResourceStorage->rootItemIndirectDrawIndexedDataPerFrame.resize(smMaxItemNums, GpuItemIndirectDrawIndexedData{});

		// 更新工作
		auto& rootConstantsPerFrame = mPipelineResourceStorage->rootConstantsPerFrame;
		rootConstantsPerFrame.deltaTime = deltaTime;
		rootConstantsPerFrame.totalTime = totalTime;
		rootConstantsPerFrame.windParameters = Math::Vector4{ 1.0f, 0.0f, 0.0f, 10.0f };
		rootConstantsPerFrame.finalRTResolution    = Math::Vector2{ static_cast<float>(mOutputWidth), static_cast<float>(mOutputHeight) };
		rootConstantsPerFrame.finalRTResolutionInv = 1.0f / rootConstantsPerFrame.finalRTResolution;

		UpdateCameras();
		UpdateItems();
		UpdateSky();
		UpdateLights();
		UpdateShadow();
		
		mTerrainRenderer->Update();
	}

	void RenderEngine::UpdateCameras() {
		auto& rootConstantsPerFrame = mPipelineResourceStorage->rootConstantsPerFrame;

		static uint32_t cameraJitterFrameIndex = 0u;
		static uint32_t taaSampleCount = 16u;
		auto jitter = Math::Jitter::GetJitter(cameraJitterFrameIndex, taaSampleCount, Math::Vector2{
			static_cast<float>(mOutputWidth), static_cast<float>(mOutputHeight)
		});

		ECS::Entity::Foreach([&](ECS::Camera& camera, ECS::Transform& transform) {
			if (camera.cameraType == ECS::CameraType::EditorCamera) {
				auto& gpuPreviousEditorCamera = rootConstantsPerFrame.previousEditorCamera;
				auto& gpuCurrentEditorCamera = rootConstantsPerFrame.currentEditorCamera;
				if (!mFrameTracker->IsFirstFrame()) {
					gpuPreviousEditorCamera = gpuCurrentEditorCamera;
				}
				gpuCurrentEditorCamera.position = transform.worldPosition;
				gpuCurrentEditorCamera.lookUp = Math::Vector4{ camera.lookUp, 1.0f };
				gpuCurrentEditorCamera.view = camera.viewMatrix.Transpose();
				gpuCurrentEditorCamera.projection = camera.projMatrix.Transpose();
				gpuCurrentEditorCamera.viewProjection = camera.viewProjMatrix.Transpose();
				gpuCurrentEditorCamera.inverseView = camera.viewMatrix.Inverse().Transpose();
				gpuCurrentEditorCamera.inverseProjection = camera.projMatrix.Inverse().Transpose();
				gpuCurrentEditorCamera.nearPlane = camera.frustum.nearZ;
				gpuCurrentEditorCamera.farPlane = camera.frustum.farZ;
				gpuCurrentEditorCamera.foVV = camera.frustum.fovY;
				gpuCurrentEditorCamera.aspectRatio = camera.frustum.aspect;
				gpuCurrentEditorCamera.jitter = jitter.jitterMatrix;
				gpuCurrentEditorCamera.uvJitter = jitter.uvJitter;
				gpuCurrentEditorCamera.viewProjectionJitter = (camera.viewProjMatrix * jitter.jitterMatrix).Transpose();
				Math::Frustum::BuildFrustumPlanes(camera.viewMatrix * camera.projMatrix, gpuCurrentEditorCamera.planes);
				if (mFrameTracker->IsFirstFrame()) {
					gpuPreviousEditorCamera = gpuCurrentEditorCamera;
				}
			}

			if (camera.mainCamera == true && camera.cameraType == ECS::CameraType::RenderCamera) {
				auto& gpuPreviousRenderCamera = rootConstantsPerFrame.previousRenderCamera;
				auto& gpuCurrentRenderCamera = rootConstantsPerFrame.currentRenderCamera;
				if (!mFrameTracker->IsFirstFrame()) {
					gpuPreviousRenderCamera = gpuCurrentRenderCamera;
				}
				gpuCurrentRenderCamera.position = transform.worldPosition;
				gpuCurrentRenderCamera.lookUp = Math::Vector4{ camera.lookUp, 1.0f };
				gpuCurrentRenderCamera.view = camera.viewMatrix.Transpose();
				gpuCurrentRenderCamera.projection = camera.projMatrix.Transpose();
				gpuCurrentRenderCamera.viewProjection = camera.viewProjMatrix.Transpose();
				gpuCurrentRenderCamera.inverseView = camera.viewMatrix.Inverse().Transpose();
				gpuCurrentRenderCamera.inverseProjection = camera.projMatrix.Inverse().Transpose();
				gpuCurrentRenderCamera.nearPlane = camera.frustum.nearZ;
				gpuCurrentRenderCamera.farPlane = camera.frustum.farZ;
				gpuCurrentRenderCamera.foVV = camera.frustum.fovY;
				gpuCurrentRenderCamera.aspectRatio = camera.frustum.aspect;
				gpuCurrentRenderCamera.jitter = jitter.jitterMatrix;
				gpuCurrentRenderCamera.uvJitter = jitter.uvJitter;
				gpuCurrentRenderCamera.viewProjectionJitter = (camera.viewProjMatrix * jitter.jitterMatrix).Transpose();
				Math::Frustum::BuildFrustumPlanes(camera.viewMatrix * camera.projMatrix, gpuCurrentRenderCamera.planes);
				if (mFrameTracker->IsFirstFrame()) {
					gpuPreviousRenderCamera = gpuCurrentRenderCamera;
				}
			}
		});

		cameraJitterFrameIndex++;
	}

	void RenderEngine::UpdateItems() {
		std::atomic<int32_t> atomicIndex = -1;
		ECS::Entity::Foreach([&](ECS::Transform& transform, ECS::MeshRenderer& meshRenderer) {
			int32_t index = ++atomicIndex;

			GpuItemData itemData{};
			Math::Matrix4 cpuWorldMatrix = transform.GetWorldMatrix();
			Math::Matrix4 gpuWorldMatrix = cpuWorldMatrix.Transpose();
			itemData.prevModelTrans = mFrameTracker->IsFirstFrame() ? gpuWorldMatrix : transform.prevWorldMatrix;
			itemData.currModelTrans = gpuWorldMatrix;
			itemData.boundingBoxInWorldSpace = meshRenderer.mesh->GetBoundingBox().transformBy(cpuWorldMatrix);
			// transform.prevWorldMatrix = gpuWorldMatrix;

			GpuItemIndirectDrawIndexedData indirectDrawIndexedData{};
			indirectDrawIndexedData.vertexBufferView = meshRenderer.mesh->GetVertexBuffer()->GetVBDescriptor();
			indirectDrawIndexedData.indexBufferView = meshRenderer.mesh->GetIndexBuffer()->GetIBDescriptor();
			indirectDrawIndexedData.drawIndexedArguments.BaseVertexLocation = 0u;
			indirectDrawIndexedData.drawIndexedArguments.StartInstanceLocation = 0u;
			indirectDrawIndexedData.drawIndexedArguments.InstanceCount = 1u;
			indirectDrawIndexedData.drawIndexedArguments.StartIndexLocation = 0u;
			indirectDrawIndexedData.drawIndexedArguments.IndexCountPerInstance = meshRenderer.mesh->GetIndexCount();

			mPipelineResourceStorage->rootItemDataPerFrame.at(index) = std::move(itemData);
			mPipelineResourceStorage->rootItemIndirectDrawIndexedDataPerFrame.at(index) = std::move(indirectDrawIndexedData);
		});
		mPipelineResourceStorage->rootItemNumsPerFrame = atomicIndex + 1;
	}

	void RenderEngine::UpdateShadow() {
		mCascadeShadowPass.Update(this);
	}

	void RenderEngine::UpdateSky() {
		ECS::Entity::ForeachInCurrentThread([&](ECS::Transform& transform, ECS::Sky& sky) {

			// https://www.gamedev.net/forums/topic/671214-simple-solar-radiance-calculation/

			Math::Vector3 sunDirection = transform.GetDirection();

			/*				  sun
			*				  /|  (thetaS)
			*				 / |
			*				/  |
			*	(elevation) ----
			*/
			float thetaS = std::acos(std::clamp(sunDirection.y, 0.0001f, 1.0f));
			float elevation = DirectX::XM_PIDIV2 - thetaS;
			float turbidity = sky.turbidity;

			uint32_t totalSampleCount = sky.skySpectrum.GetSamples().size();

			// Vertical sample angle. For one ray it's just equal to elevation.
			float theta = thetaS;

			// Angle between the sun direction and sample direction.
			// Since we have only one sample for simplicity, the angle is 0.
			float gamma = 0.0;

			// We compute spectrum for the middle ray at the center of the Sun's disk.
			// For simplicity, we ignore limb darkening.
			for (uint64_t i = 0; i < totalSampleCount; ++i) {
				ArHosekSkyModelState* skyState = arhosekskymodelstate_alloc_init(thetaS, turbidity, sky.groundAlbedoSpectrum[i]);
				float wavelength = Math::Mix(sky.skySpectrum.LowestWavelength(), sky.skySpectrum.HighestWavelength(), (float)i / float(totalSampleCount));
				sky.skySpectrum[i] = float(arhosekskymodel_solar_radiance(skyState, theta, gamma, wavelength));
				arhosekskymodelstate_free(skyState);
				skyState = nullptr;
			}

			Math::Vector3 sunLuminance = sky.skySpectrum.ToRGB();
			Math::Vector3 sunIlluminance = sunLuminance * ECS::SunSolidAngle; // Dividing by 1 / PDF

			// Found it on internet, without it the illuminance is too small.
			// Account for luminous efficacy, coordinate system scaling (100, wtf???)
			float multiplier = 2.5f;
			sky.sunIlluminance = sunIlluminance * multiplier;
			sky.sunLuminance = sunLuminance * multiplier;
			
			// Sun Light
			auto& gpuLightData = mPipelineResourceStorage->rootLightDataPerFrame.emplace_back();
			gpuLightData.position = Math::Vector4{ sunDirection, ECS::SunDiskArea };
			gpuLightData.color = sky.sunIlluminance;
			gpuLightData.type = std::underlying_type<ECS::LightType>::type(ECS::LightType::Sun);
			gpuLightData.intensity = 1.0f;
			gpuLightData.radiance = gpuLightData.color * gpuLightData.intensity;
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
		RenderContext renderContext{
			mSelectedDisplay,
			mDevice.get(),
			mShaderManger.get(),
			mCommandSignatureManger.get(),
			mSharedMemAllocator.get(),
			mRenderGraph->GetPipelineResourceStorage(),
			mResourceStateTracker.get(),
			nullptr,
			mFrameTracker.get()
		};

		auto rootDataAllocation = LinearAllocation{};
		// RootConstantsDataPerFrame
		size_t rootConstantsPerFrameByteSize = sizeof(RootConstantsPerFrame);
		rootDataAllocation = mSharedMemAllocator->Allocate(rootConstantsPerFrameByteSize);
		memcpy(rootDataAllocation.cpuAddress, &mPipelineResourceStorage->rootConstantsPerFrame, rootConstantsPerFrameByteSize);
		mPipelineResourceStorage->rootConstantsPerFrameAddress = rootDataAllocation.gpuAddress;

		// RootGpuLightDataPerFrame
		size_t lightDataByteSize = sizeof(GpuLightData) * mPipelineResourceStorage->rootLightDataPerFrame.size();
		rootDataAllocation = mSharedMemAllocator->Allocate(lightDataByteSize);
		memcpy(rootDataAllocation.cpuAddress, mPipelineResourceStorage->rootLightDataPerFrame.data(), lightDataByteSize);
		mPipelineResourceStorage->rootLightDataPerFrameAddress = rootDataAllocation.gpuAddress;

		{
			// Update ItemIndirectDrawIndexedPerFrame
			for (uint32_t i = 0; i < mPipelineResourceStorage->rootItemNumsPerFrame; i++) {
				auto& indirectDrawIndexedData = mPipelineResourceStorage->rootItemIndirectDrawIndexedDataPerFrame[i];
				indirectDrawIndexedData.frameDataAddress = mPipelineResourceStorage->rootConstantsPerFrameAddress;
				indirectDrawIndexedData.lightDataAddress = mPipelineResourceStorage->rootLightDataPerFrameAddress;
				indirectDrawIndexedData.passDataAddress;	// 这里的passDataAddress应该由Material的提供
				indirectDrawIndexedData.itemDataAddress = mDeferredItemDataBuffer->GetGpuAddress() + i * sizeof(GpuItemData);
			}

			// Upload ItemDataPerFrame & ItemIndirectDrawIndexedDataPerFrame
			auto commandList = mCommandListAllocator->AllocateGraphicsCommandList();
			CommandBuffer commandBuffer{ commandList.Get(), &renderContext };
			{
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch += commandBuffer.TransitionImmediately(mDeferredItemDataBuffer, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(mDeferredItemDataBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(mDeferredItemIndirectDrawIndexedDataBuffer, GHL::EResourceState::CopyDestination);
				barrierBatch += commandBuffer.TransitionImmediately(mDeferredItemIndirectDrawIndexedDataBuffer->GetCounterBuffer(), GHL::EResourceState::CopyDestination);
				commandBuffer.FlushResourceBarrier(barrierBatch);
			}
			commandBuffer.UploadBufferRegion(mDeferredItemDataBuffer, 0u, mPipelineResourceStorage->rootItemDataPerFrame.data(), mPipelineResourceStorage->rootItemNumsPerFrame * sizeof(GpuItemData));
			commandBuffer.UploadBufferRegion(mDeferredItemIndirectDrawIndexedDataBuffer, 0u, mPipelineResourceStorage->rootItemIndirectDrawIndexedDataPerFrame.data(), mPipelineResourceStorage->rootItemNumsPerFrame * sizeof(GpuItemIndirectDrawIndexedData));
			commandBuffer.ClearCounterBuffer(mDeferredItemDataBuffer, mPipelineResourceStorage->rootItemNumsPerFrame);
			commandBuffer.ClearCounterBuffer(mDeferredItemIndirectDrawIndexedDataBuffer, mPipelineResourceStorage->rootItemNumsPerFrame);
			{
				auto barrierBatch = GHL::ResourceBarrierBatch{};
				barrierBatch += commandBuffer.TransitionImmediately(mDeferredItemDataBuffer, GHL::EResourceState::NonPixelShaderAccess);
				barrierBatch += commandBuffer.TransitionImmediately(mDeferredItemDataBuffer->GetCounterBuffer(), GHL::EResourceState::NonPixelShaderAccess);
				barrierBatch += commandBuffer.TransitionImmediately(mDeferredItemIndirectDrawIndexedDataBuffer, GHL::EResourceState::NonPixelShaderAccess);
				barrierBatch += commandBuffer.TransitionImmediately(mDeferredItemIndirectDrawIndexedDataBuffer->GetCounterBuffer(), GHL::EResourceState::NonPixelShaderAccess);
				commandBuffer.FlushResourceBarrier(barrierBatch);
			}

			commandList->Close();
			mGraphicsQueue->ExecuteCommandList(commandList->D3DCommandList());
		}

		mRenderGraph->Execute();

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
				auto commandList = mCommandListAllocator->AllocateGraphicsCommandList();
				auto* descriptorHeap = mDescriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
				commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
				CommandBuffer commandBuffer{ commandList.Get(), &renderContext };

				commandBuffer.PIXBeginEvent("OutputBackBufferPass");

				auto& heightMapAtlasWrap = mTerrainRenderer->GetFarTerrainHeightMapAtlas()->GetTextureAtlas();

				mOutputBackBufferPassData.finalOutputMapIndex = mFinalOutput->GetSRDescriptor()->GetHeapIndex();
				// mOutputBackBufferPassData.finalOutputMapIndex = heightMapAtlasWrap.Get()->GetSRDescriptor()->GetHeapIndex();
				auto passDataAlloc = mSharedMemAllocator->Allocate(sizeof(OutputBackBufferPassData));
				memcpy(passDataAlloc.cpuAddress, &mOutputBackBufferPassData, sizeof(OutputBackBufferPassData));

				auto  currentBackBufferIndex = mSwapChain->GetCurrentBackBufferIndex();
				auto* currentBackBuffer = mBackBuffers.at(currentBackBufferIndex).get();
				{
					auto barrierBatch = commandBuffer.TransitionImmediately(currentBackBuffer, GHL::EResourceState::RenderTarget);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}
				commandBuffer.ClearRenderTarget(currentBackBuffer);
				commandBuffer.SetRenderTarget(currentBackBuffer);
				commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, static_cast<uint16_t>(mOutputWidth), static_cast<uint16_t>(mOutputHeight) });
				commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u,  static_cast<uint16_t>(mOutputWidth), static_cast<uint16_t>(mOutputHeight) });
				commandBuffer.SetGraphicsRootSignature();
				commandBuffer.SetGraphicsPipelineState("OutputBackBuffer");
				commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
				commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
				commandBuffer.SetVertexBuffer(0u, mOutputQuadMesh->GetVertexBuffer());
				commandBuffer.DrawInstanced(mOutputQuadMesh->GetVertexCount(), 1u, 0u, 0u);
				{
					auto barrierBatch = commandBuffer.TransitionImmediately(currentBackBuffer, GHL::EResourceState::Present);
					commandBuffer.FlushResourceBarrier(barrierBatch);
				}
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
			nullptr,
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
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, 
				static_cast<uint16_t>(mOutputWidth), static_cast<uint16_t>(mOutputHeight) });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, 
				static_cast<uint16_t>(mOutputWidth), static_cast<uint16_t>(mOutputHeight) });
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