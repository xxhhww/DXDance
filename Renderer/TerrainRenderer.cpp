#include "Renderer/TerrainRenderer.h"
#include "Renderer/TerrainPipelinePass.h"
#include "Renderer/TerrainBackend.h"
#include "Renderer/TerrainRuntimeVirtualTexture.h"
#include "Renderer/TerrainTextureArray.h"
#include "Renderer/TerrainTextureAtlas.h"
#include "Renderer/TerrainTextureAtlasTileCache.h"
#include "Renderer/RenderEngine.h"

#include "Tools/MemoryStream.h"
#include "Tools/StrUtil.h"

#include "Math/Int.h"

#include <fstream>

namespace Renderer {

	TerrainRenderer::TerrainRenderer(RenderEngine* renderEngine)
	: mRenderEngine(renderEngine) {}
	
	TerrainRenderer::~TerrainRenderer() {}

	void TerrainRenderer::Initialize() {

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
			mFarTerrainHeightMapAtlas = std::make_unique<TerrainTextureAtlas>(this, dirname + "FarTerrainHeightMapAtlas.ret", 25u);

			// FarCache
			mFarTerrainTextureAtlasTileCache = std::make_unique<TerrainTextureAtlasTileCache>(625u, 25u);

			// TextureArray
			mNearTerrainAlbedoArray = std::make_unique<TerrainTextureArray>(this, dirname + "NearTerrainAlbedoArray.ret");
			mNearTerrainNormalArray = std::make_unique<TerrainTextureArray>(this, dirname + "NearTerrainNormalArray.ret");
		}

		// 创建并初始化GPU对象
		{
			auto* device = mRenderEngine->mDevice.get();
			auto* renderGraph = mRenderEngine->mRenderGraph.get();
			auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
			auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
			auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

			Renderer::BufferDesc _TerrainLodDescriptorBufferDesc{};
			_TerrainLodDescriptorBufferDesc.stride = sizeof(TerrainLodDescriptor);
			_TerrainLodDescriptorBufferDesc.size = _TerrainLodDescriptorBufferDesc.stride * mTerrainLodDescriptors.size();
			_TerrainLodDescriptorBufferDesc.usage = GHL::EResourceUsage::Default;
			_TerrainLodDescriptorBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_TerrainLodDescriptorBufferDesc.initialState = GHL::EResourceState::Common;
			_TerrainLodDescriptorBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::AnyShaderAccess;
			mTerrainLodDescriptorBuffer = resourceAllocator->Allocate(device, _TerrainLodDescriptorBufferDesc, descriptorAllocator, nullptr);

			renderGraph->ImportResource("TerrainLodDescriptor", mTerrainLodDescriptorBuffer);
			resourceStateTracker->StartTracking(mTerrainLodDescriptorBuffer);

			Renderer::BufferDesc _TerrainNodeDescriptorBufferDesc{};
			_TerrainNodeDescriptorBufferDesc.stride = sizeof(TerrainNodeDescriptor);
			_TerrainNodeDescriptorBufferDesc.size = _TerrainNodeDescriptorBufferDesc.stride * mTerrainNodeDescriptors.size();
			_TerrainNodeDescriptorBufferDesc.usage = GHL::EResourceUsage::Default;
			_TerrainNodeDescriptorBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_TerrainNodeDescriptorBufferDesc.initialState = GHL::EResourceState::Common;
			_TerrainNodeDescriptorBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::AnyShaderAccess;
			mTerrainNodeDescriptorBuffer = resourceAllocator->Allocate(device, _TerrainNodeDescriptorBufferDesc, descriptorAllocator, nullptr);

			renderGraph->ImportResource("TerrainNodeDescriptor", mTerrainNodeDescriptorBuffer);
			resourceStateTracker->StartTracking(mTerrainNodeDescriptorBuffer);
		}

		// 地形后台线程，负责资源调度
		mTerrainBackend = std::make_unique<TerrainBackend>(this, mTerrainSetting, mTerrainLodDescriptors, mTerrainNodeDescriptors, mTerrainNodeRuntimeStates);

		mTerrainPipelinePass = std::make_unique<TerrainPipelinePass>(this);
		mTerrainPipelinePass->Initialize();
	}

	void TerrainRenderer::AddPass() {
		mTerrainPipelinePass->AddPass();
	}

	void TerrainRenderer::Update() {
		// 如果是第一帧，需要同步加载地形数据(当前摄像机位置附近 + 最高级LOD)
		auto* frameTracker = mRenderEngine->mFrameTracker.get();
		if (frameTracker->IsFirstFrame()) {
			mTerrainBackend->Preload();
		}
	}

}