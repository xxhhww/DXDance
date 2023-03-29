#include "Texture.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace Renderer {

	Texture::Texture(
		const GHL::Device* device,
		const TextureDesc& textureDesc,
		uint8_t backBufferCount,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator
	)
	: mDevice(device)
	, mTextureDesc(textureDesc)
	, mBackBufferCount(backBufferCount)
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator) {
		
		ASSERT_FORMAT(mTextureDesc.usage == GHL::EResourceUsage::Default, "Texture Usage Must be Default");

		if (mTextureDesc.reserved) {
			ASSERT_FORMAT(mHeapAllocator != nullptr, "Texture Created Reserved, HeapAllocator is nullptr");
		}

		ResolveResourceDesc();

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		// HeapAllocatorΪ�գ�����Ĭ�Ϸ�ʽ������Դ
		if (mHeapAllocator == nullptr) {
			// ��Ĭ�Ϸ�ʽ������Buffer
			HRASSERT(mDevice->D3DDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&mResourceDesc,
				GHL::GetResourceStates(mInitialStates),
				&mTextureDesc.clearVaule,
				IID_PPV_ARGS(&mResource)
			));
		}
		else {
			if (!mTextureDesc.reserved) {
				// �Է��÷�ʽ������Buffer

				// �ڶ��Ϸ���
				mHeapAllocation = mHeapAllocator->Allocate(mResourceSizeInBytes);

				// �Է��÷�ʽ������Buffer
				HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
					mHeapAllocation->heap->D3DHeap(),
					mHeapAllocation->heapOffset,
					&mResourceDesc,
					GHL::GetResourceStates(mInitialStates),
					&mTextureDesc.clearVaule,
					IID_PPV_ARGS(&mResource)
				));
			}
			else {
				ASSERT_FORMAT(false, "Unsupport Reserved Resource!");
				/*
				// �Ա�����ʽ������Buffer
				HRASSERT(mDevice->D3DDevice()->CreateReservedResource(
					&mResourceDesc,
					mInitialStates,
					&mTextureDesc.clearVaule,
					IID_PPV_ARGS(&mResource)
				));

				UINT subresourceCount = mResourceDesc.MipLevels;
				mTiling.resize(subresourceCount);
				mDevice->D3DDevice()->GetResourceTiling(mResource.Get(), &mNumTilesTotal, &mPackedMipInfo, &mTileShape, &subresourceCount, 0, &mTiling[0]);

				// ����Feedback
				mFeedbackDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
				mFeedbackDesc.Format = DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE;
				mFeedbackDesc.MipLevels = mResourceDesc.MipLevels;
				mFeedbackDesc.Alignment = 0u;
				mFeedbackDesc.DepthOrArraySize = mResourceDesc.DepthOrArraySize;
				mFeedbackDesc.Height = mResourceDesc.Height;
				mFeedbackDesc.Width = mResourceDesc.Width;
				mFeedbackDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
				mFeedbackDesc.SampleDesc.Count = 1u;
				mFeedbackDesc.SampleDesc.Quality = 0u;
				mFeedbackDesc.SamplerFeedbackMipRegion.Height = mTileShape.HeightInTexels;
				mFeedbackDesc.SamplerFeedbackMipRegion.Width = mTileShape.WidthInTexels;
				mFeedbackDesc.SamplerFeedbackMipRegion.Depth = mTileShape.DepthInTexels;

				D3D12_HEAP_PROPERTIES heapProperties;
				heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;

				// ��Ĭ�Ϸ�ʽ����Feedback
				HRASSERT(mDevice->D3DDevice()->CreateCommittedResource2(
					&heapProperties,
					D3D12_HEAP_FLAG_NONE,
					&mFeedbackDesc,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					nullptr,
					nullptr,
					IID_PPV_ARGS(&mResource)
				));
				*/
			}
		}
	}

	Texture::Texture(
		const GHL::Device* device,
		const TextureDesc& textureDesc,
		uint8_t backBufferCount,
		PoolDescriptorAllocator* descriptorAllocator,
		const GHL::Heap* heap,
		size_t heapOffset
	)
	: mDevice(device)
	, mTextureDesc(textureDesc)
	, mBackBufferCount(backBufferCount)
	, mDescriptorAllocator(descriptorAllocator) {

		ASSERT_FORMAT(mTextureDesc.usage == GHL::EResourceUsage::Default, "Texture Usage Must be Default");
		ASSERT_FORMAT(heap->GetUsage() == GHL::EResourceUsage::Default, "Heap Usage Must be Default");

		ResolveResourceDesc();

		// �Է��÷�ʽ������Buffer
		HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
			heap->D3DHeap(),
			heapOffset,
			&mResourceDesc,
			GHL::GetResourceStates(mInitialStates),
			&mTextureDesc.clearVaule,
			IID_PPV_ARGS(&mResource)
		));
	}

	Texture::~Texture() {

	}

	void Texture::ResolveResourceDesc() {
		mInitialStates = mTextureDesc.initialState;
		mExpectedStates = mTextureDesc.initialState | mTextureDesc.expectedState;

		mResourceDesc.Dimension = GHL::GetD3DTextureDimension(mTextureDesc.dimension);
		mResourceDesc.Format = mTextureDesc.format;
		mResourceDesc.MipLevels = mTextureDesc.mipLevals;
		mResourceDesc.Alignment = 0u;
		mResourceDesc.DepthOrArraySize = (mTextureDesc.dimension == GHL::ETextureDimension::Texture3D) ? mTextureDesc.depth : mTextureDesc.arraySize;
		mResourceDesc.Height = mTextureDesc.height;
		mResourceDesc.Width = mTextureDesc.width;
		mResourceDesc.Flags = D3D12_RESOURCE_FLAG_NONE;
		mResourceDesc.SampleDesc.Count = mTextureDesc.sampleCount;
		mResourceDesc.SampleDesc.Quality = 0u;
		
		if (mTextureDesc.reserved) {
			mResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_64KB_UNDEFINED_SWIZZLE;
		}
		else {
			mResourceDesc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
		}

		if (HasAllFlags(mExpectedStates, GHL::EResourceState::RenderTarget)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}

		if (HasAllFlags(mExpectedStates, GHL::EResourceState::DepthRead) ||
			HasAllFlags(mExpectedStates, GHL::EResourceState::DepthWrite)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			if (!HasAllFlags(mExpectedStates, GHL::EResourceState::PixelShaderAccess) &&
				!HasAllFlags(mExpectedStates, GHL::EResourceState::NonPixelShaderAccess)) {
				mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}

		if (HasAllFlags(mExpectedStates, GHL::EResourceState::UnorderedAccess)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		D3D12_RESOURCE_ALLOCATION_INFO allocInfo = mDevice->D3DDevice()->GetResourceAllocationInfo(mDevice->GetNodeMask(), 1, &mResourceDesc);
		mResourceSizeInBytes = allocInfo.SizeInBytes;
	}

	const GHL::DescriptorHandle* Texture::GetDSDescriptor(const TextureSubResourceDesc& subDesc) {
		ASSERT_FORMAT(HasAllFlags(mExpectedStates, GHL::EResourceState::DepthRead) ||
			HasAllFlags(mExpectedStates, GHL::EResourceState::DepthWrite), "Unsupport DSDescriptor");

		if (mDSDescriptors.find(subDesc) != mDSDescriptors.end()) {
			return mDSDescriptors.at(subDesc).Get();
		}

		// DSView
		D3D12_DEPTH_STENCIL_VIEW_DESC dsvDesc{};
		dsvDesc.Format = mTextureDesc.format;

		if (mTextureDesc.dimension == GHL::ETextureDimension::Texture1D) {
			if (mTextureDesc.arraySize> 1u) {
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
		else if (mTextureDesc.dimension == GHL::ETextureDimension::Texture2D) {
			if (mTextureDesc.arraySize > 1u) {
				if (mTextureDesc.sampleCount > 1u) {
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
				if (mTextureDesc.sampleCount > 1u) {
					dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2DMS;
				}
				else {
					dsvDesc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
					dsvDesc.Texture2D.MipSlice = subDesc.firstMip;
				}
			}
		}

		mDSDescriptors[subDesc] = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
		mDevice->D3DDevice()->CreateDepthStencilView(mResource.Get(), &dsvDesc, *mDSDescriptors[subDesc].Get());
		return mDSDescriptors[subDesc].Get();

	}

	const GHL::DescriptorHandle* Texture::GetSRDescriptor(const TextureSubResourceDesc& subDesc) {
		ASSERT_FORMAT(HasAllFlags(mExpectedStates, GHL::EResourceState::PixelShaderAccess) ||
			HasAllFlags(mExpectedStates, GHL::EResourceState::NonPixelShaderAccess), "Unsupport SRDescriptor");

		if (mSRDescriptors.find(subDesc) != mSRDescriptors.end()) {
			return mSRDescriptors.at(subDesc).Get();
		}

		// SRView
		D3D12_SHADER_RESOURCE_VIEW_DESC srvDesc{};
		srvDesc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
		srvDesc.Format = mTextureDesc.format;

		if (mTextureDesc.dimension == GHL::ETextureDimension::Texture1D) {

			if (mTextureDesc.arraySize > 1u) {
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
		else if (mTextureDesc.dimension == GHL::ETextureDimension::Texture2D) {

			if (mTextureDesc.arraySize > 1u) {

				if (HasAnyFlag(mTextureDesc.miscFlag, GHL::ETextureMiscFlag::CubeTexture)) {

					if (mTextureDesc.arraySize > 6u) {
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
		else if (mTextureDesc.dimension == GHL::ETextureDimension::Texture3D) {

			srvDesc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE3D;
			srvDesc.Texture3D.MostDetailedMip = subDesc.firstMip;
			srvDesc.Texture3D.MipLevels = subDesc.mipCount;

		}

		mSRDescriptors[subDesc] = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDevice->D3DDevice()->CreateShaderResourceView(mResource.Get(), &srvDesc, *mSRDescriptors[subDesc].Get());
		return mSRDescriptors[subDesc].Get();

	}

	const GHL::DescriptorHandle* Texture::GetRTDescriptor(const TextureSubResourceDesc& subDesc) {
		ASSERT_FORMAT(HasAllFlags(mExpectedStates, GHL::EResourceState::RenderTarget), "Unsupport RTDescriptor");

		if (mRTDescriptors.find(subDesc) != mRTDescriptors.end()) {
			return mRTDescriptors.at(subDesc).Get();
		}

		// RTView
		D3D12_RENDER_TARGET_VIEW_DESC rtvDesc{};
		rtvDesc.Format = mTextureDesc.format;

		if (mTextureDesc.dimension == GHL::ETextureDimension::Texture1D) {
			if (mTextureDesc.arraySize > 1u) {
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
		else if (mTextureDesc.dimension == GHL::ETextureDimension::Texture2D) {
			if (mTextureDesc.arraySize > 1u) {
				if (mTextureDesc.sampleCount > 1u) {
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
				if (mTextureDesc.sampleCount > 1u) {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2DMS;
				}
				else {
					rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE2D;
					rtvDesc.Texture2D.MipSlice = subDesc.firstMip;
				}
			}
		}
		else if (mTextureDesc.dimension == GHL::ETextureDimension::Texture3D) {
			rtvDesc.ViewDimension = D3D12_RTV_DIMENSION_TEXTURE3D;
			rtvDesc.Texture3D.MipSlice = subDesc.firstMip;
			rtvDesc.Texture3D.FirstWSlice = subDesc.firstSlice;
			rtvDesc.Texture3D.WSize = subDesc.sliceCount;
		}

		mRTDescriptors[subDesc] = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
		mDevice->D3DDevice()->CreateRenderTargetView(mResource.Get(), &rtvDesc, *mRTDescriptors[subDesc].Get());
		return mRTDescriptors[subDesc].Get();

	}

	const GHL::DescriptorHandle* Texture::GetUADescriptor(const TextureSubResourceDesc& subDesc) {
		ASSERT_FORMAT(HasAllFlags(mExpectedStates, GHL::EResourceState::UnorderedAccess), "Unsupport UADescriptor");

		if (mUADescriptors.find(subDesc) != mUADescriptors.end()) {
			return mUADescriptors.at(subDesc).Get();
		}

		// UAView
		D3D12_UNORDERED_ACCESS_VIEW_DESC uavDesc{};
		uavDesc.Format = mTextureDesc.format;

		if (mTextureDesc.dimension == GHL::ETextureDimension::Texture1D) {
			if (mTextureDesc.arraySize > 1u) {
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
		else if (mTextureDesc.dimension == GHL::ETextureDimension::Texture2D) {
			if (mTextureDesc.arraySize > 1u) {
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
		else if (mTextureDesc.dimension == GHL::ETextureDimension::Texture3D) {
			uavDesc.ViewDimension = D3D12_UAV_DIMENSION_TEXTURE3D;
			uavDesc.Texture3D.MipSlice = subDesc.firstMip;
			uavDesc.Texture3D.FirstWSlice = subDesc.firstSlice;
			uavDesc.Texture3D.WSize = subDesc.sliceCount;
		}

		mUADescriptors[subDesc] = mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		mDevice->D3DDevice()->CreateUnorderedAccessView(mResource.Get(), nullptr, &uavDesc, *mUADescriptors[subDesc].Get());
		return mUADescriptors[subDesc].Get();

	}

}