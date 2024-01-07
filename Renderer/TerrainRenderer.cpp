#include "Renderer/TerrainRenderer.h"
#include "Renderer/TerrainPipelinePass.h"
#include "Renderer/TerrainBackend.h"
#include "Renderer/TerrainRuntimeVirtualTexture.h"
#include "Renderer/TerrainTextureArray.h"
#include "Renderer/TerrainTextureAtlas.h"
#include "Renderer/TerrainTiledTexture.h"
#include "Renderer/RenderEngine.h"

#include "Tools/MemoryStream.h"
#include "Tools/StrUtil.h"

#include <fstream>

namespace Renderer {

	TerrainRenderer::TerrainRenderer(RenderEngine* renderEngine)
	: mRenderEngine(renderEngine) {
		mTerrainBackend = std::make_unique<TerrainBackend>(this);
		mTerrainPipelinePass = std::make_unique<TerrainPipelinePass>(this);
	}
	
	TerrainRenderer::~TerrainRenderer() {}

	void TerrainRenderer::Initialize() {
		
		std::string dirname = "E:/TerrainOfflineTask/001/Runtime/";

		// ��ȡ��������
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

			mFarTerrainHeightMapAtlas = std::make_unique<TerrainTextureAtlas>(this, dirname + "FarTerrainHeightMapAtlas.ret");

			mNearTerrainTiledHeightMap = std::make_unique<TerrainTiledTexture>(this, dirname + "NearTerrainTiledHeightMap.ret");

			mNearTerrainAlbedoArray = std::make_unique<TerrainTextureArray>(this, dirname + "NearTerrainAlbedoArray.ret");
			mNearTerrainNormalArray = std::make_unique<TerrainTextureArray>(this, dirname + "NearTerrainNormalArray.ret");
		}

		// ���κ�̨�̣߳�������Դ����
		mHasPreloaded = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mHasPreloaded != nullptr, "CreateEvent Failed");
		mTerrainBackend->Initialize();
		
		// ������ȾPass
		mTerrainPipelinePass->Initialize();
	}

	void TerrainRenderer::AddPass() {
		mTerrainPipelinePass->AddPass();
	}

	void TerrainRenderer::Update() {
		// ����ǵ�һ֡����Ҫͬ�����ص�������(��ǰ�����λ�ø��� + ��߼�LOD)
		auto* frameTracker = mRenderEngine->mFrameTracker.get();
		const auto& rootConstantsPerFrame = mRenderEngine->mPipelineResourceStorage->rootConstantsPerFrame;
		if (frameTracker->IsFirstFrame()) {
			// Ԥ���ص�������


			// ������ɺ�֪ͨ��̨�̣߳�������Դ����
			::SetEvent(mHasPreloaded);
		}
	}

}