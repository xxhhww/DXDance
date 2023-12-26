#include "Renderer/TextureAtlas.h"

namespace Renderer {

	TextureAtlasTile::TextureAtlasTile(
		const GHL::Device* device,
		const ResourceFormat& subResourceFormat,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator)
	: mDevice(device)
	, mResourceFormat(subResourceFormat)
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

		const auto& textureDesc = mResourceFormat.GetTextureDesc();
		D3D12_CLEAR_VALUE d3dClearValue = GHL::GetD3DClearValue(textureDesc.clearVaule, textureDesc.format);
		bool canUseClearValue = mResourceFormat.CanUseClearValue();

		ASSERT_FORMAT(textureDesc.usage == GHL::EResourceUsage::Default, "Resource Usage Must Be Default");
		ASSERT_FORMAT(textureDesc.createdMethod == GHL::ECreatedMethod::Placed, "Create Method Must Be Placed");

		mHeapAllocation = mHeapAllocator->AllocateEx(mResourceFormat.GetSizeInBytes());
		HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
			mHeapAllocation->heap->D3DHeap(),
			mHeapAllocation->heapOffset,
			&mResourceFormat.D3DResourceDesc(),
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
		const auto& textureDesc = mResourceFormat.GetTextureDesc();

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

	TextureAtlasFileDescriptor::TextureAtlasFileDescriptor(const std::string& filepath)
	: filepath(filepath) {}

	uint32_t TextureAtlasFileDescriptor::GetCompressionFormat() const {
		return 0;
	}

	const TextureAtlasFileDescriptor::SubresourceOffset& TextureAtlasFileDescriptor::GetSubresourceOffset(uint32_t index) const {
		return SubresourceOffset{};
	}

	TextureAtlas::TextureAtlas(
		const GHL::Device* device,
		const ResourceFormat& resourceFormat,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator,
		IDStorageFactory* dstorageFactory,
		const std::string& filepath)
	: mDevice(device)
	, mSubResourceFormat(resourceFormat)
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {
		mTextureAtlasFileDescriptor = std::make_unique<TextureAtlasFileDescriptor>(filepath);
		for (uint32_t i = 0; i < mTextureAtlasFileDescriptor->subresourceNums; i++) {
			mTextureAtlasTiles.emplace_back(new TextureAtlasTile(device, resourceFormat, descriptorAllocator, heapAllocator));
		}

		HRASSERT(dstorageFactory->OpenFile(Tool::StrUtil::UTF8ToWString(filepath).c_str(), IID_PPV_ARGS(&mDStorageFile)));
	}

	TextureAtlas::~TextureAtlas() {}

}