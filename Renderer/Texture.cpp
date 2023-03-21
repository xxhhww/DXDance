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
			ASSERT_FORMAT(mHeapAllocator != nullptr, "Texture Created Reserved, HeapAllocator Is Nullptr");
		}

		ResolveResourceDesc();

		D3D12_HEAP_PROPERTIES heapProperties{};
		heapProperties.Type = D3D12_HEAP_TYPE_DEFAULT;
		// HeapAllocator为空，则以默认方式创建资源
		if (mHeapAllocator == nullptr) {
			// 以默认方式创建该Buffer
			HRASSERT(mDevice->D3DDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&mResourceDesc,
				mInitialStates,
				&mTextureDesc.clearVaule,
				IID_PPV_ARGS(&mResource)
			));
		}
		else {
			if (!mTextureDesc.reserved) {
				// 以放置方式创建该Buffer

				// 在堆上分配
				mHeapAllocations.emplace_back(mHeapAllocator->Allocate(mResourceSizeInBytes));

				// 以放置方式创建该Buffer
				HRASSERT(mDevice->D3DDevice()->CreatePlacedResource(
					mHeapAllocations[0]->heap->D3DHeap(),
					mHeapAllocations[0]->heapOffset,
					&mResourceDesc,
					mInitialStates,
					&mTextureDesc.clearVaule,
					IID_PPV_ARGS(&mResource)
				));
			}
			else {
				// 以保留方式创建该Buffer
				HRASSERT(mDevice->D3DDevice()->CreateReservedResource(
					&mResourceDesc,
					mInitialStates,
					&mTextureDesc.clearVaule,
					IID_PPV_ARGS(&mResource)
				));

				UINT subresourceCount = mResourceDesc.MipLevels;
				mTiling.resize(subresourceCount);
				mDevice->D3DDevice()->GetResourceTiling(mResource.Get(), &mNumTilesTotal, &mPackedMipInfo, &mTileShape, &subresourceCount, 0, &mTiling[0]);

				// 创建Feedback
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

				// 以默认方式创建Feedback
				HRASSERT(mDevice->D3DDevice()->CreateCommittedResource2(
					&heapProperties,
					D3D12_HEAP_FLAG_NONE,
					&mFeedbackDesc,
					D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
					nullptr,
					nullptr,
					IID_PPV_ARGS(&mResource)
				));

			}
		}

		// 创建描述符
		CreateDescriptor();

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

	}

	Texture::~Texture() {

	}

	void Texture::ResolveResourceDesc() {
		mInitialStates = GHL::GetResourceStates(mTextureDesc.initialState);
		mExpectedStates = GHL::GetResourceStates(mTextureDesc.initialState | mTextureDesc.expectedState);

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

		if (HasAllFlags(mTextureDesc.expectedState, GHL::EResourceState::RenderTarget)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_RENDER_TARGET;
		}

		if (HasAllFlags(mTextureDesc.expectedState, GHL::EResourceState::DepthRead) ||
			HasAllFlags(mTextureDesc.expectedState, GHL::EResourceState::DepthWrite)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;

			if (!HasAllFlags(mTextureDesc.expectedState, GHL::EResourceState::PixelShaderAccess) &&
				!HasAllFlags(mTextureDesc.expectedState, GHL::EResourceState::NonPixelShaderAccess)) {
				mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_DENY_SHADER_RESOURCE;
			}
		}

		if (HasAllFlags(mTextureDesc.expectedState, GHL::EResourceState::UnorderedAccess)) {
			mResourceDesc.Flags |= D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
		}

		D3D12_RESOURCE_ALLOCATION_INFO allocInfo = mDevice->D3DDevice()->GetResourceAllocationInfo(mDevice->GetNodeMask(), 1, &mResourceDesc);
		mResourceSizeInBytes = allocInfo.SizeInBytes;

	}

	void Texture::CreateDescriptor() {

	}

}