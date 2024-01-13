#include "Renderer/TerrainTextureAtlas.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RenderEngine.h"

#include "Tools/StrUtil.h"

namespace Renderer {

	TerrainTextureAtlas::TerrainTextureAtlas(TerrainRenderer* renderer, const std::string& filepath, uint32_t tileCountPerAxis)
	: mRenderer(renderer) 
	, mReTextureFileFormat(filepath) 
	, mTileCountPerAxis(tileCountPerAxis)
	, mTileCount(mTileCountPerAxis* mTileCountPerAxis)
	, mTextureAtlasSize(GetTileSize()* mTileCountPerAxis) {

		const auto& fileHeader = mReTextureFileFormat.GetFileHeader();

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageFactory = renderEngine->mDStorageFactory.get();

		auto* renderGraph = renderEngine->mRenderGraph.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		TextureDesc _TextureAtlasDesc{};
		_TextureAtlasDesc.width = mTextureAtlasSize;
		_TextureAtlasDesc.height = mTextureAtlasSize;
		_TextureAtlasDesc.format = (DXGI_FORMAT)fileHeader.dxgiFormat;
		_TextureAtlasDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::AnyShaderAccess;
		_TextureAtlasDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		mTextureAtlas = resourceAllocator->Allocate(device, _TextureAtlasDesc, descriptorAllocator, nullptr);
		mTextureAtlas->SetDebugName(mReTextureFileFormat.GetFilename());

		renderGraph->ImportResource(mReTextureFileFormat.GetFilename(), mTextureAtlas);
		resourceStateTracker->StartTracking(mTextureAtlas);

		// ´´½¨DStorageFile
		mDStorageFile = std::make_unique<GHL::DirectStorageFile>(dstorageFactory, filepath);
	}

}