#include "Renderer/RuntimeVirtualTextureAtlas.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RenderEngine.h"

namespace Renderer {

	RuntimeVirtualTextureAtlas::RuntimeVirtualTextureAtlas(TerrainRenderer* terrainRenderer, DXGI_FORMAT dxgiFormat, const std::string& name)
	: mRenderer(terrainRenderer) 
	, mDxgiFormat(dxgiFormat)
	, mTileSizeNoPadding(mRenderer->mTerrainSetting.smRvtTileSizeNoPadding)
	, mPaddingSize(mRenderer->mTerrainSetting.smRvtTilePaddingSize)
	, mTileCountPerAxis(mRenderer->mTerrainSetting.smRvtTileCountPerAxisInAtlas)
	, mTileCount(mTileCountPerAxis * mTileCountPerAxis)
	, mTextureAtlasSize(GetTileSizeWithPadding() * mTileCountPerAxis) {

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageFactory = renderEngine->mDStorageFactory.get();

		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		TextureDesc _PhysicalTextureDesc{};
		_PhysicalTextureDesc.width = mTextureAtlasSize;
		_PhysicalTextureDesc.height = mTextureAtlasSize;
		_PhysicalTextureDesc.format = dxgiFormat;
		_PhysicalTextureDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::PixelShaderAccess;
		_PhysicalTextureDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		mTextureAtlas = resourceAllocator->Allocate(device, _PhysicalTextureDesc, descriptorAllocator, nullptr);
		mTextureAtlas->SetDebugName(name);

		renderGraph->ImportResource(name, mPhysicalTexture);
		resourceStateTracker->StartTracking(mPhysicalTexture);
	}

}