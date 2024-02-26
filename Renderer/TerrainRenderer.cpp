#include "Renderer/TerrainRenderer.h"
#include "Renderer/TerrainPipelinePass.h"
#include "Renderer/TerrainBackend.h"
#include "Renderer/TerrainTextureArray.h"
#include "Renderer/TerrainTextureAtlas.h"
#include "Renderer/TerrainTextureAtlasTileCache.h"
#include "Renderer/RuntimeVirtualTextureBackend.h"
#include "Renderer/RuntimeVirtualTextureAtlas.h"
#include "Renderer/RenderEngine.h"

#include "Tools/MemoryStream.h"
#include "Tools/StrUtil.h"

#include "Math/Int.h"

#include <fstream>

namespace Renderer {

	TerrainRenderer::TerrainRenderer(RenderEngine* renderEngine)
	: mRenderEngine(renderEngine) 
	, mQueuedFeedbackReadbacks(mRenderEngine->mFrameTracker->GetMaxSize()) {}
	
	TerrainRenderer::~TerrainRenderer() {}

	void TerrainRenderer::Initialize() {

		auto* resourceStorage = mRenderEngine->mPipelineResourceStorage;

		auto& finalOutputDesc = resourceStorage->GetResourceByName("FinalOutput")->GetTexture()->GetResourceFormat().GetTextureDesc();


		std::string dirname = "E:/TerrainOfflineTask/001/Runtime/";

		// 读取地形数据
		{
			std::ifstream terrainLodDescriptorStream(dirname + "TerrainLodDescriptor.bin", std::ios::binary | std::ios::in);
			Tool::InputMemoryStream terrainLodDescriptorInputMem(terrainLodDescriptorStream);
			size_t terrainLodDescriptorSize = 0u;
			terrainLodDescriptorInputMem.Read(terrainLodDescriptorSize);
			mTerrainLodDescriptors.resize(terrainLodDescriptorSize);
			terrainLodDescriptorInputMem.Read(mTerrainLodDescriptors.data(), sizeof(TerrainLodDescriptor) * terrainLodDescriptorSize);

			std::ifstream terrainNodeDescriptorStream(dirname + "TerrainNodeDescriptor.bin", std::ios::binary | std::ios::in);
			Tool::InputMemoryStream terrainNodeDescriptorInputMem(terrainNodeDescriptorStream);
			size_t terrainNodeDescriptorSize = 0u;
			terrainNodeDescriptorInputMem.Read(terrainNodeDescriptorSize);
			mTerrainNodeDescriptors.resize(terrainNodeDescriptorSize);
			terrainNodeDescriptorInputMem.Read(mTerrainNodeDescriptors.data(), sizeof(TerrainNodeDescriptor) * terrainNodeDescriptorSize);

			mTerrainNodeRuntimeStates.resize(terrainNodeDescriptorSize);

			// Far
			mFarTerrainHeightMapAtlas = std::make_unique<TerrainTextureAtlas>(this, dirname + "FarTerrainHeightMapAtlas.ret", mTerrainSetting.smFarTerrainTextureAtlasTileCountPerAxis);
			mFarTerrainAlbedoMapAtlas = std::make_unique<TerrainTextureAtlas>(this, dirname + "FarTerrainAlbedoMapAtlas.ret", mTerrainSetting.smFarTerrainTextureAtlasTileCountPerAxis);
			mFarTerrainNormalMapAtlas = std::make_unique<TerrainTextureAtlas>(this, dirname + "FarTerrainNormalMapAtlas.ret", mTerrainSetting.smFarTerrainTextureAtlasTileCountPerAxis);

			// FarCache
			mFarTerrainTextureAtlasTileCache = std::make_unique<TerrainTextureAtlasTileCache>(mTerrainSetting.smFarTerrainTextureAtlasTileCountPerAxis);

			// TextureArray
			mNearTerrainAlbedoArray = std::make_unique<TerrainTextureArray>(this, dirname + "NearTerrainAlbedoArray.ret");
			mNearTerrainNormalArray = std::make_unique<TerrainTextureArray>(this, dirname + "NearTerrainNormalArray.ret");
		}

		// 创建并初始化GPU对象
		{
			auto* device = mRenderEngine->mDevice.get();
			auto* renderGraph = mRenderEngine->mRenderGraph.get();
			auto* frameTracker = mRenderEngine->mFrameTracker.get();
			auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
			auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
			auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

			Renderer::BufferDesc _TerrainLodDescriptorBufferDesc{};
			_TerrainLodDescriptorBufferDesc.stride = sizeof(TerrainLodDescriptor);
			_TerrainLodDescriptorBufferDesc.size = _TerrainLodDescriptorBufferDesc.stride * mTerrainLodDescriptors.size();
			_TerrainLodDescriptorBufferDesc.usage = GHL::EResourceUsage::Default;
			_TerrainLodDescriptorBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_TerrainLodDescriptorBufferDesc.initialState = GHL::EResourceState::Common;
			_TerrainLodDescriptorBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mTerrainLodDescriptorBuffer = resourceAllocator->Allocate(device, _TerrainLodDescriptorBufferDesc, descriptorAllocator, nullptr);
			mTerrainLodDescriptorBuffer->SetDebugName("TerrainLodDescriptor");

			renderGraph->ImportResource("TerrainLodDescriptor", mTerrainLodDescriptorBuffer);
			resourceStateTracker->StartTracking(mTerrainLodDescriptorBuffer);

			Renderer::BufferDesc _TerrainNodeDescriptorBufferDesc{};
			_TerrainNodeDescriptorBufferDesc.stride = sizeof(TerrainNodeDescriptor);
			_TerrainNodeDescriptorBufferDesc.size = _TerrainNodeDescriptorBufferDesc.stride * mTerrainNodeDescriptors.size();
			_TerrainNodeDescriptorBufferDesc.usage = GHL::EResourceUsage::Default;
			_TerrainNodeDescriptorBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_TerrainNodeDescriptorBufferDesc.initialState = GHL::EResourceState::Common;
			_TerrainNodeDescriptorBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess | GHL::EResourceState::UnorderedAccess;
			mTerrainNodeDescriptorBuffer = resourceAllocator->Allocate(device, _TerrainNodeDescriptorBufferDesc, descriptorAllocator, nullptr);
			mTerrainNodeDescriptorBuffer->SetDebugName("TerrainNodeDescriptor");

			renderGraph->ImportResource("TerrainNodeDescriptor", mTerrainNodeDescriptorBuffer);
			resourceStateTracker->StartTracking(mTerrainNodeDescriptorBuffer);

			TextureDesc _TerrainFeedbackMapDesc{};
			_TerrainFeedbackMapDesc.width = finalOutputDesc.width / mTerrainSetting.smTerrainFeedbackScale;
			_TerrainFeedbackMapDesc.height = finalOutputDesc.height / mTerrainSetting.smTerrainFeedbackScale;
			_TerrainFeedbackMapDesc.format = DXGI_FORMAT_R16G16B16A16_UINT;
			_TerrainFeedbackMapDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::CopySource;
			_TerrainFeedbackMapDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
			mTerrainFeedbackMap = resourceAllocator->Allocate(device, _TerrainFeedbackMapDesc, descriptorAllocator, nullptr);
			mTerrainFeedbackMap->SetDebugName("TerrainFeedback");

			renderGraph->ImportResource("TerrainFeedback", mTerrainFeedbackMap);
			resourceStateTracker->StartTracking(mTerrainFeedbackMap);

			TextureDesc _TerrainFeedbackDepthMapDesc{};
			_TerrainFeedbackDepthMapDesc.width = finalOutputDesc.width / mTerrainSetting.smTerrainFeedbackScale;
			_TerrainFeedbackDepthMapDesc.height = finalOutputDesc.height / mTerrainSetting.smTerrainFeedbackScale;
			_TerrainFeedbackDepthMapDesc.format = DXGI_FORMAT_D32_FLOAT;
			_TerrainFeedbackDepthMapDesc.expectedState = GHL::EResourceState::DepthWrite;
			_TerrainFeedbackDepthMapDesc.clearVaule = GHL::DepthStencilClearValue{ 1.0f, 0u };
			mTerrainFeedbackDepthMap = resourceAllocator->Allocate(device, _TerrainFeedbackDepthMapDesc, descriptorAllocator, nullptr);
			mTerrainFeedbackDepthMap->SetDebugName("TerrainFeedbackDepth");

			renderGraph->ImportResource("TerrainFeedbackDepth", mTerrainFeedbackDepthMap);
			resourceStateTracker->StartTracking(mTerrainFeedbackDepthMap);

			BufferDesc _TerrainReadbackBufferDesc{};
			_TerrainReadbackBufferDesc.size = GetRequiredIntermediateSize(mTerrainFeedbackMap->D3DResource(), 0, 1);
			_TerrainReadbackBufferDesc.usage = GHL::EResourceUsage::ReadBack;
			_TerrainReadbackBufferDesc.initialState = GHL::EResourceState::CopyDestination;
			_TerrainReadbackBufferDesc.expectedState = _TerrainReadbackBufferDesc.initialState;
			mTerrainFeedbackReadbackBuffers.resize(frameTracker->GetMaxSize());
			for (uint32_t i = 0; i < mTerrainFeedbackReadbackBuffers.size(); i++) {
				mTerrainFeedbackReadbackBuffers[i] = resourceAllocator->Allocate(device, _TerrainReadbackBufferDesc, descriptorAllocator, nullptr);
				mTerrainFeedbackReadbackBuffers[i]->SetDebugName("TerrainFeedbackReadBack" + std::to_string(i));
				renderGraph->ImportResource("TerrainFeedbackReadBack" + std::to_string(i), mTerrainFeedbackReadbackBuffers[i]);
				resourceStateTracker->StartTracking(mTerrainFeedbackReadbackBuffers[i]);
			}

			// 除以2直到smRvtTileCountPerAxisInPage0Level不能再被2整除
			mMaxPageLevel = 0u;
			uint32_t tempCount = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			mRvtLookupPageTables.emplace_back(mMaxPageLevel, mTerrainSetting.smRvtTileCountPerAxisInPage0Level);
			while (tempCount != 1 && tempCount % 2 == 0) {
				mMaxPageLevel++;
				tempCount /= 2;

				mRvtLookupPageTables.emplace_back(mMaxPageLevel, mTerrainSetting.smRvtTileCountPerAxisInPage0Level);
			}

			// LookupPageTableMap
			TextureDesc _LookupPageTableMapDesc{};
			_LookupPageTableMapDesc.width = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			_LookupPageTableMapDesc.height = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			_LookupPageTableMapDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
			_LookupPageTableMapDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::PixelShaderAccess;
			_LookupPageTableMapDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
			mRvtLookupPageTableMap = resourceAllocator->Allocate(device, _LookupPageTableMapDesc, descriptorAllocator, nullptr);
			mRvtLookupPageTableMap->SetDebugName("RvtLookupPageTableMap");
			renderGraph->ImportResource("RvtLookupPageTableMap", mRvtLookupPageTableMap);
			resourceStateTracker->StartTracking(mRvtLookupPageTableMap);

			mNearTerrainRvtAlbedoAtlas = std::make_unique<Renderer::RuntimeVirtualTextureAtlas>(this, DXGI_FORMAT_R8G8B8A8_UNORM, "NearTerrainRvtAlbedoMapAtlas");
			mNearTerrainRvtNormalAtlas = std::make_unique<Renderer::RuntimeVirtualTextureAtlas>(this, DXGI_FORMAT_R16G16B16A16_FLOAT, "NearTerrainRvtNormalMapAtlas");
			mNearTerrainRuntimeVirtualTextureAtlasTileCache = std::make_unique<Renderer::RuntimeVirtualTextureAtlasTileCache>(mTerrainSetting.smRvtTileCountPerAxisInAtlas);
		}

		// 地形后台线程，负责资源调度
		mTerrainBackend = std::make_unique<TerrainBackend>(this, mTerrainSetting, mTerrainLodDescriptors, mTerrainNodeDescriptors, mTerrainNodeRuntimeStates);

		// 实时虚拟纹理线程
		mRuntimeVirtualTextureBackend = std::make_unique<RuntimeVirtualTextureBackend>(this, mTerrainSetting);

		mTerrainPipelinePass = std::make_unique<TerrainPipelinePass>(this);
	}

	void TerrainRenderer::AddPass() {
		mTerrainPipelinePass->AddPass();
	}

	void TerrainRenderer::Update() {
		const auto& cameraPosition = mRenderEngine->mPipelineResourceStorage->rootConstantsPerFrame.currentEditorCamera.position;

		// 如果是第一帧，需要同步加载地形数据(当前摄像机位置附近 + 最高级LOD)
		static bool smFirstFrame = true;
		if (smFirstFrame) {
			mTerrainBackend->Preload();
			smFirstFrame = false;

			Math::Int2 fixedPos0 = GetFixedPosition(Math::Vector2{ cameraPosition.x, cameraPosition.z }, mTerrainSetting.smWorldMeterSizePerTileInPage0Level);
			Math::Int2 fixedPos1 = GetFixedPosition(Math::Vector2{ (float)fixedPos0.x, (float)fixedPos0.y }, mTerrainSetting.smRvtRealRectChangedViewDistance);

			mRvtRealRect = Math::Vector4{
				(float)(fixedPos1.x - mTerrainSetting.smRvtRectRadius), 
				(float)(fixedPos1.y + mTerrainSetting.smRvtRectRadius),
				(float)(2 * mTerrainSetting.smRvtRectRadius),
				(float)(2 * mTerrainSetting.smRvtRectRadius)
			};
		}
	}

	Math::Int2 TerrainRenderer::GetFixedPosition(const Math::Vector2& position, int32_t cellSize) {
		return Math::Int2{
			(int32_t)std::floor(position.x / cellSize + 0.5f) * (int32_t)cellSize,
			(int32_t)std::floor(position.y / cellSize + 0.5f) * (int32_t)cellSize
		};
	}

	void TerrainRenderer::NotifyRealRectChanged() {
		++mRvtRealRectChangedFlag;
	}

	bool TerrainRenderer::ConsumeRealRectChanged() {
		if (mRvtRealRectChangedFlag == 1u) {
			--mRvtRealRectChangedFlag;
			return true;
		}
		return false;
	}

	void TerrainRenderer::SetRealRectChangedEvnet() { 
		::SetEvent(mRvtRealRectChangedEvent); 
	}

	void TerrainRenderer::WaitRealRectChangedEvnet() { 
		::WaitForSingleObject(mRvtRealRectChangedEvent, INFINITE); 
	}

}