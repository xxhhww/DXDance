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

			mNearTerrainRvtAlbedoMapAtlas = std::make_unique<Renderer::RuntimeVirtualTextureAtlas>(this, DXGI_FORMAT_R8G8B8A8_UNORM, "NearTerrainRvtAlbedoMapAtlas");
			mNearTerrainRvtNormalMapAtlas = std::make_unique<Renderer::RuntimeVirtualTextureAtlas>(this, DXGI_FORMAT_R16G16B16A16_FLOAT, "NearTerrainRvtNormalMapAtlas");
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
		// 如果是第一帧，需要同步加载地形数据(当前摄像机位置附近 + 最高级LOD)
		static bool smFirstFrame = true;
		if (smFirstFrame) {
			mTerrainBackend->Preload();
			smFirstFrame = false;
		}
	}

}