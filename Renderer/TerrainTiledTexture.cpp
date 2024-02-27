#include "Renderer/TerrainTiledTexture.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RenderEngine.h"

#include "Tools/StrUtil.h"

namespace Renderer {

	TerrainTiledTexture::TerrainTiledTexture(TerrainRenderer* renderer, const std::string& filepath)
		: mRenderer(renderer)
		, mReTextureFileFormat(filepath) {

		const auto& fileHeader = mReTextureFileFormat.GetFileHeader();

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageFactory = renderEngine->mDStorageFactory.get();

		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		// 创建ResourceFormat
		TextureDesc _TiledTextureDesc{};
		_TiledTextureDesc.width = fileHeader.imageWidth;
		_TiledTextureDesc.height = fileHeader.imageHeight;
		_TiledTextureDesc.arraySize = fileHeader.arraySize;
		_TiledTextureDesc.mipLevals = fileHeader.mipLevels;
		_TiledTextureDesc.format = (DXGI_FORMAT)fileHeader.dxgiFormat;
		_TiledTextureDesc.createdMethod = GHL::ECreatedMethod::Reserved;
		_TiledTextureDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::AnyShaderAccess;
		_TiledTextureDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		mTiledTexture = resourceAllocator->Allocate(device, _TiledTextureDesc, descriptorAllocator, nullptr);
		mTiledTexture->SetDebugName(mReTextureFileFormat.GetFilename());

		renderGraph->ImportResource(mReTextureFileFormat.GetFilename(), mTiledTexture);
		resourceStateTracker->StartTracking(mTiledTexture);

		// 创建DStorageFile
		mDStorageFile = std::make_unique<GHL::DirectStorageFile>(dstorageFactory, filepath);
	}

}