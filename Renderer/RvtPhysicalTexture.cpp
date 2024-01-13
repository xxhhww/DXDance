#include "Renderer/RvtPhysicalTexture.h"
#include "Renderer/RuntimeVirtualTextureSystem.h"
#include "Renderer/RenderEngine.h"

namespace Renderer {

	RvtPhysicalTexture::RvtPhysicalTexture(RuntimeVirtualTextureSystem* rvtSystem)
	: mRenderEngine(rvtSystem->mRenderEngine)
	, mRvtSystem(rvtSystem) 
	, mTileSize(rvtSystem->GetTileSize()) 
	, mPaddingSize(rvtSystem->GetPaddingSize())
	, mTileCountPerAxis(15u) 
	, mTileCount(mTileCountPerAxis * mTileCountPerAxis)
	, mPhysicalTextureSize(GetTileSizeWithPadding() * mTileCountPerAxis)
	, mTileCache(mTileCount, mTileCountPerAxis) {

		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		mPhysicalTextures.resize(2u);
		TextureDesc _PhysicalTextureDesc{};
		_PhysicalTextureDesc.width  = mPhysicalTextureSize;
		_PhysicalTextureDesc.height = mPhysicalTextureSize;
		_PhysicalTextureDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		_PhysicalTextureDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::PixelShaderAccess;
		_PhysicalTextureDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		mPhysicalTextures[0] = resourceAllocator->Allocate(device, _PhysicalTextureDesc, descriptorAllocator, nullptr);
		mPhysicalTextures[0]->SetDebugName("PhysicalTextureAlbedo");

		mPhysicalTextures[1] = resourceAllocator->Allocate(device, _PhysicalTextureDesc, descriptorAllocator, nullptr);
		mPhysicalTextures[1]->SetDebugName("PhysicalTextureNormal");

		renderGraph->ImportResource("PhysicalTextureAlbedo", mPhysicalTextures[0]);
		resourceStateTracker->StartTracking(mPhysicalTextures[0]);

		renderGraph->ImportResource("PhysicalTextureNormal", mPhysicalTextures[1]);
		resourceStateTracker->StartTracking(mPhysicalTextures[1]);
	}

}