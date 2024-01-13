#include "Renderer/TerrainTextureArray.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RenderEngine.h"

#include "Tools/StrUtil.h"

namespace Renderer {

	TerrainTextureArray::TerrainTextureArray(TerrainRenderer* renderer, const std::string& filepath)
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
		TextureDesc _TextureArrayDesc{};
		_TextureArrayDesc.width = fileHeader.imageWidth;
		_TextureArrayDesc.height = fileHeader.imageHeight;
		_TextureArrayDesc.arraySize = fileHeader.arraySize;
		_TextureArrayDesc.mipLevals = fileHeader.mipLevels;
		_TextureArrayDesc.format = (DXGI_FORMAT)fileHeader.dxgiFormat;
		_TextureArrayDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::AnyShaderAccess;
		_TextureArrayDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		mTextureArray = resourceAllocator->Allocate(device, _TextureArrayDesc, descriptorAllocator, nullptr);
		mTextureArray->SetDebugName(mReTextureFileFormat.GetFilename());

		renderGraph->ImportResource(mReTextureFileFormat.GetFilename(), mTextureArray);
		resourceStateTracker->StartTracking(mTextureArray);

		// 创建DStorageFile
		mDStorageFile = std::make_unique<GHL::DirectStorageFile>(dstorageFactory, filepath);
	}

}