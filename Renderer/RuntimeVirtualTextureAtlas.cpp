#include "Renderer/RuntimeVirtualTextureAtlas.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RenderEngine.h"

namespace Renderer {

	RuntimeVirtualTextureAtlas::RuntimeVirtualTextureAtlas(TerrainRenderer* terrainRenderer, DXGI_FORMAT dxgiFormat, const std::string& name)
	: mRenderer(terrainRenderer) 
	, mTileSizeNoPadding(mRenderer->mTerrainSetting.smRvtTileSizeNoPadding)
	, mPaddingSize(mRenderer->mTerrainSetting.smRvtTilePaddingSize)
	, mTileCountPerAxis(mRenderer->mTerrainSetting.smRvtTileCountPerAxisInAtlas)
	, mTileCount(mTileCountPerAxis * mTileCountPerAxis)
	, mPhysicalTextureSize(GetTileSizeWithPadding() * mTileCountPerAxis) {

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageFactory = renderEngine->mDStorageFactory.get();

		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		TextureDesc _PhysicalTextureDesc{};
		_PhysicalTextureDesc.width = mPhysicalTextureSize;
		_PhysicalTextureDesc.height = mPhysicalTextureSize;
		_PhysicalTextureDesc.format = dxgiFormat;
		_PhysicalTextureDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::PixelShaderAccess;
		_PhysicalTextureDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		mPhysicalTexture = resourceAllocator->Allocate(device, _PhysicalTextureDesc, descriptorAllocator, nullptr);
		mPhysicalTexture->SetDebugName(name);

		renderGraph->ImportResource(name, mPhysicalTexture);
		resourceStateTracker->StartTracking(mPhysicalTexture);
	}

}