#include "Renderer/TextureAtlas.h"
#include <fstream>

namespace Renderer {

	TextureAtlasTile::TextureAtlasTile(
		const GHL::Device* device,
		TextureAtlas* atlas,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator)
	: mDevice(device)
	, mAtlas(atlas)
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {}

	TextureAtlasTile::~TextureAtlasTile() {
		if (mDescriptorAllocation != nullptr) {
			mDescriptorAllocation->Release();
			mDescriptorAllocation = nullptr;
		}
		if (mHeapAllocation != nullptr) {
			mHeapAllocation->Release();
			mHeapAllocation = nullptr;
		}
	}

	void TextureAtlasTile::Create() {
		if (mD3DResource != nullptr) {
			return;
		}

		const auto& resourceFormat = mAtlas->GetSubresourceFormat();

		const auto& textureDesc = resourceFormat.GetTextureDesc();
		D3D12_CLEAR_VALUE d3dClearValue = GHL::GetD3DClearValue(textureDesc.clearVaule, textureDesc.format);
		bool canUseClearValue = resourceFormat.CanUseClearValue();

		ASSERT_FORMAT(textureDesc.usage == GHL::EResourceUsage::Default, "Resource Usage Must Be Default");
		ASSERT_FORMAT(textureDesc.createdMethod == GHL::ECreatedMethod::Placed, "Create Method Must Be Placed");

		mHeapAllocation = mHeapAllocator->AllocateEx(resourceFormat.GetSizeInBytes());
		HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
			mHeapAllocation->heap->D3DHeap(),
			mHeapAllocation->heapOffset,
			&resourceFormat.D3DResourceDesc(),
			GHL::GetD3DResourceStates(textureDesc.initialState),
			canUseClearValue ? &d3dClearValue : nullptr,
			IID_PPV_ARGS(&mD3DResource)
		));

		CreateSRDescriptor();
	}

	void TextureAtlasTile::Release() {
		if (mDescriptorAllocation != nullptr) {
			mDescriptorAllocation->Release();
			mDescriptorAllocation = nullptr;
		}
		if (mHeapAllocation != nullptr) {
			mHeapAllocation->Release();
			mHeapAllocation = nullptr;
		}
		mD3DResource = nullptr;
	}

	void TextureAtlasTile::CreateSRDescriptor(const TextureSubResourceDesc& subDesc) {
		const auto& textureDesc = mAtlas->GetSubresourceFormat().GetTextureDesc();

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

	TextureAtlasFileDescriptor::TextureAtlasFileDescriptor(const std::string& filepath) {
		std::ifstream inFile(filepath.c_str(), std::ios::binary);
		ASSERT_FORMAT(!inFile.fail(), "File Not Exists");

		inFile.read((char*)&mFileHeader, sizeof(FileHeader));
		ASSERT_FORMAT(inFile.good(), "Unexpected Error reading header");

		mTileDataDescriptors.resize(mFileHeader.tileCount);
		inFile.read((char*)mTileDataDescriptors.data(), mTileDataDescriptors.size() * sizeof(TileDataDescriptorInFile));
		if (!inFile.good()) { ASSERT_FORMAT(false, "Unexpected Error reading tileDataDescriptors"); }
	}
	
	TextureAtlasFileDescriptor::~TextureAtlasFileDescriptor() {
	}

	TextureAtlas::TextureAtlas(
		const GHL::Device* device,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator,
		IDStorageFactory* dstorageFactory,
		const std::string& filepath)
	: mDevice(device)
	, mTextureAtlasFileDescriptor(filepath)
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {
		for (uint32_t i = 0; i < mTextureAtlasFileDescriptor.GetTileCount(); i++) {
			mTextureAtlasTiles.emplace_back(new TextureAtlasTile(device, this, descriptorAllocator, heapAllocator));
		}

		const auto& fileHeader = mTextureAtlasFileDescriptor.GetFileHeader();
		// ´´½¨SubResourceFormat
		TextureDesc tileDesc{};
		tileDesc.width = fileHeader.tileWidth;
		tileDesc.height = fileHeader.tileHeight;
		tileDesc.format = fileHeader.dxgiFormat;
		tileDesc.initialState = GHL::EResourceState::AnyShaderAccess;
		tileDesc.expectedState = GHL::EResourceState::AnyShaderAccess | GHL::EResourceState::CopyDestination;
		tileDesc.createdMethod = GHL::ECreatedMethod::Reserved;
		mSubResourceFormat = ResourceFormat{ mDevice, tileDesc };

		HRASSERT(dstorageFactory->OpenFile(Tool::StrUtil::UTF8ToWString(filepath).c_str(), IID_PPV_ARGS(&mDStorageFile)));
	}

	TextureAtlas::~TextureAtlas() {

	}

}