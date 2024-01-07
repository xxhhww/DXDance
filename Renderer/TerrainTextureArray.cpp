#include "Renderer/TerrainTextureArray.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RenderEngine.h"

#include "Tools/StrUtil.h"

namespace Renderer {

	TerrainTextureArray::TerrainTextureArray(TerrainRenderer* renderer, const std::string& filepath)
	: mRenderer(renderer)
	, mHeapAllocator(renderer->GetHeapAllocator())
	, mDescriptorAllocator(renderer->GetDescriptorAllocator()) 
	, mReTextureFileFormat(filepath) {
		mDevice = mRenderer->GetRenderEngine()->mDevice.get();

		const auto& fileHeader = mReTextureFileFormat.GetFileHeader();

		// ´´½¨ResourceFormat
		TextureDesc textureDesc{};
		textureDesc.width = fileHeader.imageWidth;
		textureDesc.height = fileHeader.imageHeight;
		textureDesc.arraySize = fileHeader.arraySize;
		textureDesc.mipLevals = fileHeader.mipLevels;
		textureDesc.usage = GHL::EResourceUsage::Default;
		textureDesc.format = (DXGI_FORMAT)fileHeader.dxgiFormat;
		textureDesc.initialState = GHL::EResourceState::AnyShaderAccess;
		textureDesc.expectedState = GHL::EResourceState::AnyShaderAccess | GHL::EResourceState::CopyDestination;
		textureDesc.createdMethod = GHL::ECreatedMethod::Reserved;
		mResourceFormat = ResourceFormat{ mDevice, textureDesc };

		auto* dstorageFactory = mRenderer->GetRenderEngine()->mUploaderEngine->GetDSFactory();
		HRASSERT(dstorageFactory->OpenFile(Tool::StrUtil::UTF8ToWString(filepath).c_str(), IID_PPV_ARGS(&mDStorageFile)));

		CreateD3DResource();
	}
	
	TerrainTextureArray::~TerrainTextureArray() {

	}

	void TerrainTextureArray::CreateD3DResource() {
		const auto& resourceName = mReTextureFileFormat.GetFilename();
		const auto& textureDesc = mResourceFormat.GetTextureDesc();
		const auto& d3dReservedResourceDesc = mResourceFormat.D3DResourceDesc();

		ASSERT_FORMAT(textureDesc.usage == GHL::EResourceUsage::Default);
		ASSERT_FORMAT(textureDesc.createdMethod == GHL::ECreatedMethod::Reserved);

		HRASSERT(mDevice->D3DDevice()->CreateReservedResource(
			&d3dReservedResourceDesc, GHL::GetD3DResourceStates(textureDesc.initialState), nullptr, IID_PPV_ARGS(&mD3DReservedResource)
		));
		mD3DReservedResource->SetName(Tool::StrUtil::UTF8ToWString(resourceName).c_str());

		uint32_t subresourceCount = mResourceFormat.SubresourceCount();
		mTiling.resize(subresourceCount);
		mDevice->D3DDevice()->GetResourceTiling(mD3DReservedResource.Get(), &mNumTilesTotal, &mPackedMipInfo, &mTileShape, &subresourceCount, 0, &mTiling[0]);
		mNumStandardMips = mPackedMipInfo.NumStandardMips;
		mNumStandardArrays = textureDesc.arraySize;
	}

}