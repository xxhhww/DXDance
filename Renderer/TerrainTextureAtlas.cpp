#include "Renderer/TerrainTextureAtlas.h"
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RenderEngine.h"

#include "Tools/StrUtil.h"

namespace Renderer {

	TerrainTextureAtlas::Tile::Tile(
		const GHL::Device* device,
		TerrainTextureAtlas* atlas,
		BuddyHeapAllocator* heapAllocator,
		PoolDescriptorAllocator* descriptorAllocator)
	: mDevice(device)
	, mTextureAtlas(atlas)
	, mHeapAllocator(heapAllocator)
	, mDescriptorAllocator(descriptorAllocator) {}
	
	TerrainTextureAtlas::Tile::~Tile() {
		if (mHeapAllocation != nullptr) {
			mHeapAllocation->Release();
			mHeapAllocation = nullptr;
		}
		if (mDescriptorAllocation != nullptr) {
			mDescriptorAllocation->Release();
			mDescriptorAllocation = nullptr;
		}
	}

	void TerrainTextureAtlas::Tile::Create() {
		ASSERT_FORMAT(mD3DResource == nullptr);

		const auto& resourceFormat = mTextureAtlas->GetSubresourceFormat();
		const auto& textureDesc = resourceFormat.GetTextureDesc();

		ASSERT_FORMAT(textureDesc.usage == GHL::EResourceUsage::Default, "Resource Usage Must Be Default");
		ASSERT_FORMAT(textureDesc.createdMethod == GHL::ECreatedMethod::Placed, "Create Method Must Be Placed");

		mHeapAllocation = mHeapAllocator->AllocateEx(resourceFormat.GetSizeInBytes());
		HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
			mHeapAllocation->heap->D3DHeap(),
			mHeapAllocation->heapOffset,
			&resourceFormat.D3DResourceDesc(),
			GHL::GetD3DResourceStates(textureDesc.initialState),
			nullptr,
			IID_PPV_ARGS(&mD3DResource)
		));

		CreateSRDescriptor();
	}

	void TerrainTextureAtlas::Tile::Release() {
		if (mHeapAllocation != nullptr) {
			mHeapAllocation->Release();
			mHeapAllocation = nullptr;
		}
		if (mDescriptorAllocation != nullptr) {
			mDescriptorAllocation->Release();
			mDescriptorAllocation = nullptr;
		}
		mD3DResource = nullptr;
	}

	void TerrainTextureAtlas::Tile::CreateSRDescriptor(const TextureSubResourceDesc& subDesc) {
		const auto& textureDesc = mTextureAtlas->GetSubresourceFormat().GetTextureDesc();

		ASSERT_FORMAT(HasAllFlags(textureDesc.expectedState, GHL::EResourceState::PixelShaderAccess) ||
			HasAllFlags(textureDesc.expectedState, GHL::EResourceState::NonPixelShaderAccess), "Unsupport SRDescriptor");

		if (mDescriptorAllocation != nullptr) {
			return;
		}

		// SRView
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = textureDesc.format;

		if (textureDesc.dimension == GHL::ETextureDimension::Texture1D) {

			if (textureDesc.arraySize > 1u) {
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1DARRAY;
				srvDesc.Texture1DArray.FirstArraySlice = subDesc.firstSlice;
				srvDesc.Texture1DArray.ArraySize = subDesc.sliceCount;
				srvDesc.Texture1DArray.MostDetailedMip = subDesc.firstMip;
				srvDesc.Texture1DArray.MipLevels = subDesc.mipCount;
			}
			else {
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE1D;
				srvDesc.Texture1D.MostDetailedMip = subDesc.firstMip;
				srvDesc.Texture1D.MipLevels = subDesc.mipCount;
			}

		}
		else if (textureDesc.dimension == GHL::ETextureDimension::Texture2D) {

			if (textureDesc.arraySize > 1u) {

				if (HasAnyFlag(textureDesc.miscFlag, GHL::ETextureMiscFlag::CubeTexture)) {

					if (textureDesc.arraySize > 6u) {
						srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBEARRAY;
						srvDesc.TextureCubeArray.First2DArrayFace = subDesc.firstSlice;
						srvDesc.TextureCubeArray.NumCubes = subDesc.sliceCount / 6u;
						srvDesc.TextureCubeArray.MostDetailedMip = subDesc.firstMip;
						srvDesc.TextureCubeArray.MipLevels = subDesc.mipCount;
					}
					else {
						srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURECUBE;
						srvDesc.TextureCube.MostDetailedMip = subDesc.firstMip;
						srvDesc.TextureCube.MipLevels = subDesc.mipCount;
					}

				}
				else {

					srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2DARRAY;
					srvDesc.Texture2DArray.FirstArraySlice = subDesc.firstSlice;
					srvDesc.Texture2DArray.ArraySize = subDesc.sliceCount;
					srvDesc.Texture2DArray.MostDetailedMip = subDesc.firstMip;
					srvDesc.Texture2DArray.MipLevels = subDesc.mipCount;

				}
			}
			else
			{
				srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
				srvDesc.Texture2D.MostDetailedMip = subDesc.firstMip;
				srvDesc.Texture2D.MipLevels = subDesc.mipCount;

			}
		}
		else if (textureDesc.dimension == GHL::ETextureDimension::Texture3D) {

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MostDetailedMip = subDesc.firstMip;
			srvDesc.Texture3D.MipLevels = subDesc.mipCount;

		}

		mDescriptorAllocation = mDescriptorAllocator->AllocateEx(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDevice->D3DDevice()->CreateShaderResourceView(mD3DResource.Get(), &srvDesc, *mDescriptorAllocation->descriptorHandle);
	}

	TerrainTextureAtlas::TerrainTextureAtlas(TerrainRenderer* renderer, const std::string& filepath) 
	: mRenderer(renderer) 
	, mHeapAllocator(renderer->GetHeapAllocator())
	, mDescriptorAllocator(renderer->GetDescriptorAllocator())
	, mReTextureFileFormat(filepath) {
		mDevice = mRenderer->GetRenderEngine()->mDevice.get();

		const auto& fileHeader = mReTextureFileFormat.GetFileHeader();

		// ´´½¨SubResourceFormat
		TextureDesc tileDesc{};
		tileDesc.width = fileHeader.tileWidth;
		tileDesc.height = fileHeader.tileHeight;
		tileDesc.format = (DXGI_FORMAT)fileHeader.dxgiFormat;
		tileDesc.initialState = GHL::EResourceState::AnyShaderAccess;
		tileDesc.expectedState = GHL::EResourceState::AnyShaderAccess | GHL::EResourceState::CopyDestination;
		tileDesc.createdMethod = GHL::ECreatedMethod::Placed;
		mSubResourceFormat = ResourceFormat{ mDevice, tileDesc };

		auto* dstorageFactory = mRenderer->GetRenderEngine()->mUploaderEngine->GetDSFactory();
		HRASSERT(dstorageFactory->OpenFile(Tool::StrUtil::UTF8ToWString(filepath).c_str(), IID_PPV_ARGS(&mDStorageFile)));

		for (uint32_t i = 0; i < mReTextureFileFormat.GetTileNums(); i++) {
			mTileDatas.emplace_back(TerrainTextureAtlas::Tile(mDevice, this, mHeapAllocator, mDescriptorAllocator));
		}
	}
	
	TerrainTextureAtlas::~TerrainTextureAtlas() {
	}

	TerrainTextureAtlas::Tile* TerrainTextureAtlas::GetTileData(uint8_t x, uint8_t y, uint8_t lod) const {
		return nullptr;
	}

}