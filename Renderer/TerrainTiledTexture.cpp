#include "Renderer/TerrainTiledTexture.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RenderEngine.h"

#include "Tools/StrUtil.h"

namespace Renderer {

	TerrainTiledTexture::TileMappingState::TileMappingState(const ReTextureFileFormat& reTextureFileFormat) {
		uint32_t subresourceNums = reTextureFileFormat.GetSubresourceNums();
		const auto& subresourceInfos = reTextureFileFormat.GetSubresourceInfos();

		mTileDatas.resize(subresourceNums);

		for (uint32_t i = 0; i < subresourceNums; i++) {
			const auto& subresourceInfo = subresourceInfos.at(i);

			mTileDatas.at(i).resize(subresourceInfo.heightTiles);

			for (uint32_t j = 0; j < subresourceInfo.heightTiles; j++) {
				mTileDatas.at(i).at(j).resize(subresourceInfo.widthTiles);
			}
		}
	}
	
	TerrainTiledTexture::TileMappingState::~TileMappingState() {

	}

	TerrainTiledTexture::TerrainTiledTexture(TerrainRenderer* renderer, const std::string& filepath)
	: mRenderer(renderer)
	, mHeapAllocator(renderer->GetHeapAllocator())
	, mDescriptorAllocator(renderer->GetDescriptorAllocator())
	, mReTextureFileFormat(filepath) {
		mDevice = mRenderer->GetRenderEngine()->mDevice.get();

		const auto& fileHeader = mReTextureFileFormat.GetFileHeader();

		TextureDesc resourceDesc{};
		resourceDesc.width = fileHeader.imageWidth;
		resourceDesc.height = fileHeader.imageHeight;
		resourceDesc.format = (DXGI_FORMAT)fileHeader.dxgiFormat;
		resourceDesc.initialState = GHL::EResourceState::AnyShaderAccess;
		resourceDesc.expectedState = GHL::EResourceState::AnyShaderAccess | GHL::EResourceState::CopyDestination;
		resourceDesc.createdMethod = GHL::ECreatedMethod::Reserved;
		mResourceFormat = ResourceFormat{ mDevice, resourceDesc };

		auto* dstorageFactory = mRenderer->GetRenderEngine()->mUploaderEngine->GetDSFactory();
		HRASSERT(dstorageFactory->OpenFile(Tool::StrUtil::UTF8ToWString(filepath).c_str(), IID_PPV_ARGS(&mDStorageFile)));

		mTileMappingStates = std::make_unique<TileMappingState>(mReTextureFileFormat);

		CreateD3DResource();
	}
	
	TerrainTiledTexture::~TerrainTiledTexture() {

	}

	void TerrainTiledTexture::CreateD3DResource() {
		const auto& resourceName = mReTextureFileFormat.GetFilename();
		const auto& textureDesc = mResourceFormat.GetTextureDesc();
		const auto& d3dReservedResourceDesc = mResourceFormat.D3DResourceDesc();

		ASSERT_FORMAT(textureDesc.usage == GHL::EResourceUsage::Default);
		ASSERT_FORMAT(textureDesc.createdMethod == GHL::ECreatedMethod::Reserved);

		HRASSERT(mDevice->D3DDevice()->CreateReservedResource(
			&d3dReservedResourceDesc, GHL::GetD3DResourceStates(textureDesc.initialState), nullptr, IID_PPV_ARGS(&mD3DResource)
		));
		mD3DResource->SetName(Tool::StrUtil::UTF8ToWString(resourceName).c_str());
	}

}