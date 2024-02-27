#include "Texture.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace Renderer {

	Texture::Texture(
		const GHL::Device* device,
		const ResourceFormat& resourceFormat,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator
	)
	: Resource(device, resourceFormat)
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {
		
		const auto& textureDesc = mResourceFormat.GetTextureDesc();
		D3D12_CLEAR_VALUE d3dClearValue = GHL::GetD3DClearValue(textureDesc.clearVaule, textureDesc.format);
		bool canUseClearValue = mResourceFormat.CanUseClearValue();

		ASSERT_FORMAT(textureDesc.usage == GHL::EResourceUsage::Default, "Texture Usage Must be Default");

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		// HeapAllocator为空，则以默认方式创建资源
		if (mHeapAllocator == nullptr && textureDesc.createdMethod != GHL::ECreatedMethod::Reserved) {
			// 以默认方式创建该Buffer
			HRASSERT(mDevice->D3DDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&mResourceFormat.D3DResourceDesc(),
				GHL::GetD3DResourceStates(textureDesc.initialState),
				canUseClearValue ? &d3dClearValue : nullptr,
				IID_PPV_ARGS(&mD3DResource)
			));
		}
		else {
			if (textureDesc.createdMethod != GHL::ECreatedMethod::Reserved) {
				// 以放置方式创建该Buffer

				// 在堆上分配
				mHeapAllocation = mHeapAllocator->Allocate(mResourceFormat.GetSizeInBytes());

				// 以放置方式创建该Buffer
				HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
					mHeapAllocation->heap->D3DHeap(),
					mHeapAllocation->heapOffset,
					&mResourceFormat.D3DResourceDesc(),
					GHL::GetD3DResourceStates(textureDesc.initialState),
					canUseClearValue ? &d3dClearValue : nullptr,
					IID_PPV_ARGS(&mD3DResource)
				));
			}
			else {
				// 以保留方式创建该Buffer
				HRASSERT(mDevice->D3DDevice()->CreateReservedResource(
					&mResourceFormat.D3DResourceDesc(),
					GHL::GetD3DResourceStates(textureDesc.initialState),
					canUseClearValue ? &d3dClearValue : nullptr,
					IID_PPV_ARGS(&mD3DResource)
				));
			}
		}
	}

	Texture::Texture(
		const GHL::Device* device,
		const ResourceFormat& resourceFormat,
		PoolDescriptorAllocator* descriptorAllocator,
		const GHL::Heap* heap,
		size_t heapOffset
	)
	: Resource(device, resourceFormat)
	, mDescriptorAllocator(descriptorAllocator) {

		const auto& textureDesc = mResourceFormat.GetTextureDesc();
		D3D12_CLEAR_VALUE d3dClearValue = GHL::GetD3DClearValue(textureDesc.clearVaule, textureDesc.format);
		bool canUseClearValue = mResourceFormat.CanUseClearValue();

		ASSERT_FORMAT(textureDesc.usage == GHL::EResourceUsage::Default, "Texture Usage Must be Default");
		ASSERT_FORMAT(heap->GetUsage() == GHL::EResourceUsage::Default, "Heap Usage Must be Default");

		// 以放置方式创建该Buffer
		HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
			heap->D3DHeap(),
			heapOffset,
			&mResourceFormat.D3DResourceDesc(),
			GHL::GetD3DResourceStates(textureDesc.initialState),
			canUseClearValue ? &d3dClearValue : nullptr,
			IID_PPV_ARGS(&mD3DResource)
		));
	}

	Texture::Texture(
		const GHL::Device* device, 
		ID3D12Resource* backBuffer,
		PoolDescriptorAllocator* descriptorAllocator)
	: Resource(device, ResourceFormat{}) 
	, mDescriptorAllocator(descriptorAllocator) {
		mD3DResource = backBuffer;
		mResourceFormat.SetBackBufferStates();
	}

	Texture::~Texture() {
		int i = 32;
	}

	const GHL::DescriptorHandle* Texture::GetDSDescriptor(const TextureSubResourceDesc& subDesc) {

		const auto& textureDesc = mResourceFormat.GetTextureDesc();

		ASSERT_FORMAT(HasAllFlags(textureDesc.expectedState, GHL::EResourceState::DepthRead) ||
			HasAllFlags(textureDesc.expectedState, GHL::EResourceState::DepthWrite), "Unsupport DSDescriptor");

		if (mDSDescriptors.find(subDesc) != mDSDescriptors.end()) {
			return mDSDescriptors.at(subDesc).Get();
		}

		// DSView
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = textureDesc.format;

		if (textureDesc.dimension == GHL::ETextureDimension::Texture1D) {
			if (textureDesc.arraySize> 1u) {
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1DARRAY;
				dsvDesc.Texture1DArray.FirstArraySlice = subDesc.firstSlice;
				dsvDesc.Texture1DArray.ArraySize = subDesc.sliceCount;
				dsvDesc.Texture1DArray.MipSlice = subDesc.firstMip;
			}
			else {
				dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE1D;
				dsvDesc.Texture1D.MipSlice = subDesc.firstMip;
			}
		}
		else if (textureDesc.dimension == GHL::ETextureDimension::Texture2D) {
			if (textureDesc.arraySize > 1u) {
				if (textureDesc.sampleCount > 1u) {
					dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMSARRAY;
					dsvDesc.Texture2DMSArray.FirstArraySlice = subDesc.firstSlice;
					dsvDesc.Texture2DMSArray.ArraySize = subDesc.sliceCount;
				}
				else {
					dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DARRAY;
					dsvDesc.Texture2DArray.FirstArraySlice = subDesc.firstSlice;
					dsvDesc.Texture2DArray.ArraySize = subDesc.sliceCount;
					dsvDesc.Texture2DArray.MipSlice = subDesc.firstMip;
				}
			}
			else {
				if (textureDesc.sampleCount > 1u) {
					dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
				}
				else {
					dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
					dsvDesc.Texture2D.MipSlice = subDesc.firstMip;
				}
			}
		}

		mDSDescriptors[subDesc] = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		mDevice->D3DDevice()->CreateDepthStencilView(mD3DResource.Get(), &dsvDesc, *mDSDescriptors[subDesc].Get());
		return mDSDescriptors[subDesc].Get();

	}

	const GHL::DescriptorHandle* Texture::GetSRDescriptor(const TextureSubResourceDesc& subDesc) {

		const auto& textureDesc = mResourceFormat.GetTextureDesc();

		ASSERT_FORMAT(HasAllFlags(textureDesc.expectedState, GHL::EResourceState::PixelShaderAccess) ||
			HasAllFlags(textureDesc.expectedState, GHL::EResourceState::NonPixelShaderAccess), "Unsupport SRDescriptor");

		if (mSRDescriptors.find(subDesc) != mSRDescriptors.end()) {
			return mSRDescriptors.at(subDesc).Get();
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

		mSRDescriptors[subDesc] = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDevice->D3DDevice()->CreateShaderResourceView(mD3DResource.Get(), &srvDesc, *mSRDescriptors[subDesc].Get());
		return mSRDescriptors[subDesc].Get();
	}

	const GHL::DescriptorHandle* Texture::GetRTDescriptor(const TextureSubResourceDesc& subDesc) {

		const auto& textureDesc = mResourceFormat.GetTextureDesc();

		// ASSERT_FORMAT(HasAllFlags(textureDesc.expectedState, GHL::EResourceState::RenderTarget), "Unsupport RTDescriptor");

		if (mRTDescriptors.find(subDesc) != mRTDescriptors.end()) {
			return mRTDescriptors.at(subDesc).Get();
		}

		// RTView
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = textureDesc.format;

		if (textureDesc.dimension == GHL::ETextureDimension::Texture1D) {
			if (textureDesc.arraySize > 1u) {
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
				rtvDesc.Texture1DArray.FirstArraySlice = subDesc.firstSlice;
				rtvDesc.Texture1DArray.ArraySize = subDesc.sliceCount;
				rtvDesc.Texture1DArray.MipSlice = subDesc.firstMip;
			}
			else {
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
				rtvDesc.Texture1D.MipSlice = subDesc.firstMip;
			}
		}
		else if (textureDesc.dimension == GHL::ETextureDimension::Texture2D) {
			if (textureDesc.arraySize > 1u) {
				if (textureDesc.sampleCount > 1u) {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
					rtvDesc.Texture2DMSArray.FirstArraySlice = subDesc.firstSlice;
					rtvDesc.Texture2DMSArray.ArraySize = subDesc.sliceCount;
				}
				else {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
					rtvDesc.Texture2DArray.FirstArraySlice = subDesc.firstSlice;
					rtvDesc.Texture2DArray.ArraySize = subDesc.sliceCount;
					rtvDesc.Texture2DArray.MipSlice = subDesc.firstMip;
				}
			}
			else {
				if (textureDesc.sampleCount > 1u) {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
				}
				else {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
					rtvDesc.Texture2D.MipSlice = subDesc.firstMip;
				}
			}
		}
		else if (textureDesc.dimension == GHL::ETextureDimension::Texture3D) {
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			rtvDesc.Texture3D.MipSlice = subDesc.firstMip;
			rtvDesc.Texture3D.FirstWSlice = subDesc.firstSlice;
			rtvDesc.Texture3D.WSize = subDesc.sliceCount;
		}

		mRTDescriptors[subDesc] = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		mDevice->D3DDevice()->CreateRenderTargetView(mD3DResource.Get(), &rtvDesc, *mRTDescriptors[subDesc].Get());
		return mRTDescriptors[subDesc].Get();

	}

	const GHL::DescriptorHandle* Texture::GetUADescriptor(const TextureSubResourceDesc& subDesc) {

		const auto& textureDesc = mResourceFormat.GetTextureDesc();

		ASSERT_FORMAT(HasAllFlags(textureDesc.expectedState, GHL::EResourceState::UnorderedAccess), "Unsupport UADescriptor");

		if (mUADescriptors.find(subDesc) != mUADescriptors.end()) {
			return mUADescriptors.at(subDesc).Get();
		}

		// UAView
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = textureDesc.format;

		if (textureDesc.dimension == GHL::ETextureDimension::Texture1D) {
			if (textureDesc.arraySize > 1u) {
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1DARRAY;
				uavDesc.Texture1DArray.FirstArraySlice = subDesc.firstSlice;
				uavDesc.Texture1DArray.ArraySize = subDesc.sliceCount;
				uavDesc.Texture1DArray.MipSlice = subDesc.firstMip;
			}
			else {
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE1D;
				uavDesc.Texture1D.MipSlice = subDesc.firstMip;
			}
		}
		else if (textureDesc.dimension == GHL::ETextureDimension::Texture2D) {
			if (textureDesc.arraySize > 1u) {
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2DARRAY;
				uavDesc.Texture2DArray.FirstArraySlice = subDesc.firstSlice;
				uavDesc.Texture2DArray.ArraySize = subDesc.sliceCount;
				uavDesc.Texture2DArray.MipSlice = subDesc.firstMip;
			}
			else {
				uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE2D;
				uavDesc.Texture2D.MipSlice = subDesc.firstMip;
			}
		}
		else if (textureDesc.dimension == GHL::ETextureDimension::Texture3D) {
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = subDesc.firstMip;
			uavDesc.Texture3D.FirstWSlice = subDesc.firstSlice;
			uavDesc.Texture3D.WSize = subDesc.sliceCount;
		}

		mUADescriptors[subDesc] = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDevice->D3DDevice()->CreateUnorderedAccessView(mD3DResource.Get(), nullptr, &uavDesc, *mUADescriptors[subDesc].Get());
		return mUADescriptors[subDesc].Get();

	}

	void Texture::BindSRDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, const TextureSubResourceDesc& subDesc) {
		const auto& textureDesc = mResourceFormat.GetTextureDesc();

		ASSERT_FORMAT(HasAllFlags(textureDesc.expectedState, GHL::EResourceState::PixelShaderAccess) ||
			HasAllFlags(textureDesc.expectedState, GHL::EResourceState::NonPixelShaderAccess), "Unsupport SRDescriptor");

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
		mDevice->D3DDevice()->CreateShaderResourceView(mD3DResource.Get(), &srvDesc, cpuHandle);
	}

	void Texture::BindRTDescriptor(D3D12_CPU_DESCRIPTOR_HANDLE cpuHandle, const TextureSubResourceDesc& subDesc) {
		
		const auto& textureDesc = mResourceFormat.GetTextureDesc();

		ASSERT_FORMAT(HasAllFlags(textureDesc.expectedState, GHL::EResourceState::RenderTarget), "Unsupport RTDescriptor");

		// RTView
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = textureDesc.format;

		if (textureDesc.dimension == GHL::ETextureDimension::Texture1D) {
			if (textureDesc.arraySize > 1u) {
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1DARRAY;
				rtvDesc.Texture1DArray.FirstArraySlice = subDesc.firstSlice;
				rtvDesc.Texture1DArray.ArraySize = subDesc.sliceCount;
				rtvDesc.Texture1DArray.MipSlice = subDesc.firstMip;
			}
			else {
				rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE1D;
				rtvDesc.Texture1D.MipSlice = subDesc.firstMip;
			}
		}
		else if (textureDesc.dimension == GHL::ETextureDimension::Texture2D) {
			if (textureDesc.arraySize > 1u) {
				if (textureDesc.sampleCount > 1u) {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMSARRAY;
					rtvDesc.Texture2DMSArray.FirstArraySlice = subDesc.firstSlice;
					rtvDesc.Texture2DMSArray.ArraySize = subDesc.sliceCount;
				}
				else {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DARRAY;
					rtvDesc.Texture2DArray.FirstArraySlice = subDesc.firstSlice;
					rtvDesc.Texture2DArray.ArraySize = subDesc.sliceCount;
					rtvDesc.Texture2DArray.MipSlice = subDesc.firstMip;
				}
			}
			else {
				if (textureDesc.sampleCount > 1u) {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
				}
				else {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
					rtvDesc.Texture2D.MipSlice = subDesc.firstMip;
				}
			}
		}
		else if (textureDesc.dimension == GHL::ETextureDimension::Texture3D) {
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			rtvDesc.Texture3D.MipSlice = subDesc.firstMip;
			rtvDesc.Texture3D.FirstWSlice = subDesc.firstSlice;
			rtvDesc.Texture3D.WSize = subDesc.sliceCount;
		}
		mDevice->D3DDevice()->CreateRenderTargetView(mD3DResource.Get(), &rtvDesc, cpuHandle);
	}

}