#include "Renderer/RvtTiledTexture.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/TerrainSystem.h"

namespace Renderer {

	RvtTiledTexture::RvtTiledTexture(TerrainSystem* terrainSystem)
	: mRenderEngine(terrainSystem->mRenderEngine) 
	, mTerrainSystem(terrainSystem) {

		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		// ÔÓÏî
		{
			mAllTileCount = mTileCountPerAxis * mTileCountPerAxis;

			mTiledTextureWidth = mTileCountPerAxis * GetTileSizeWithPadding();
			mTiledTextureHeight = mTiledTextureWidth;

			mTileCache.Create(mTileCountPerAxis * mTileCountPerAxis);
		}

		// ´´½¨TiledTexture(AlbedoHeight¡¢Normal)
		{
			mTiledMaps.resize(2u);
			TextureDesc _TiledTextureDesc{};
			_TiledTextureDesc.width = mTiledTextureWidth;
			_TiledTextureDesc.height = mTiledTextureHeight;
			_TiledTextureDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_TiledTextureDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::PixelShaderAccess;
			_TiledTextureDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
			mTiledMaps[0] = resourceAllocator->Allocate(device, _TiledTextureDesc, descriptorAllocator, nullptr);
			mTiledMaps[0]->SetDebugName("TiledTextureAlbedoHeight");

			mTiledMaps[1] = resourceAllocator->Allocate(device, _TiledTextureDesc, descriptorAllocator, nullptr);
			mTiledMaps[1]->SetDebugName("TiledTextureNormal");

			renderGraph->ImportResource("TiledTextureAlbedoHeight", mTiledMaps[0]);
			resourceStateTracker->StartTracking(mTiledMaps[0]);

			renderGraph->ImportResource("TiledTextureNormal", mTiledMaps[1]);
			resourceStateTracker->StartTracking(mTiledMaps[1]);
		}
	}

	RvtTiledTexture::~RvtTiledTexture() {

	}

}