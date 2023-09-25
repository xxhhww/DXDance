#include "Renderer/StreamVirtualTexture.h"

namespace Renderer {

	StreamVirtualTexture::StreamVirtualTexture(
		const GHL::Device* device,
		TileUploader* tileUploader,
		RingFrameTracker* mainRingFrameTracker,
		ResourceAllocator* mainResourceAllocator,
		PoolDescriptorAllocator* mainPoolDescriptorAllocator,
		BuddyHeapAllocator* mainBuddyheapAllocator,
		const XeTexureFormat& fileFormat,
		std::unique_ptr<FileHandle>&& fileHandle)
	: mDevice(device)
	, mTileUploader(tileUploader)
	, mMainRingFrameTracker(mainRingFrameTracker)
	, mMainResourceAllocator(mainResourceAllocator)
	, mMainPoolDescriptorAllocator(mainPoolDescriptorAllocator)
	, mMainBuddyHeapAllocator(mainBuddyheapAllocator)
	, mFileFormat(fileFormat)
	, mFileHandle(std::move(fileHandle)) {

		ResourceFormat resourceFormat{ mDevice, mFileFormat.ConvertTextureDesc() };
		auto& textureDesc = resourceFormat.GetTextureDesc();
		ASSERT_FORMAT(textureDesc.createdMethod == GHL::ECreatedMethod::Reserved, "SoftwareVirtualTexture Must Create Reserved");

		mInternalTexture = mMainResourceAllocator->Allocate(device, textureDesc, mMainPoolDescriptorAllocator, mMainBuddyHeapAllocator);

		mPackedMipsFileOffset = mFileFormat.GetPackedMipFileOffset(&mPackedMipsNumBytes, &mPackedMipsUncompressedSize);

		const auto& d3dResourceDesc = resourceFormat.D3DResourceDesc();
		uint32_t subresourceCount = resourceFormat.SubresourceCount();
		mTiling.resize(subresourceCount);
		mDevice->D3DDevice()->GetResourceTiling(mInternalTexture->D3DResource(), &mNumTilesTotal, &mPackedMipInfo, &mTileShape, &subresourceCount, 0, &mTiling[0]);
		mNumStandardMips = mPackedMipInfo.NumStandardMips;
		// mTileMappingState = std::make_unique<TileMappingState>(mNumStandardMips, mTiling);

		// 创建Feedback Texture
		{
			mFeedbackResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			mFeedbackResourceDesc.Format = DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE;
			mFeedbackResourceDesc.MipLevels = d3dResourceDesc.MipLevels;
			mFeedbackResourceDesc.Alignment = 0u;
			mFeedbackResourceDesc.DepthOrArraySize = d3dResourceDesc.DepthOrArraySize;
			mFeedbackResourceDesc.Height = d3dResourceDesc.Height;
			mFeedbackResourceDesc.Width = d3dResourceDesc.Width;
			mFeedbackResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			mFeedbackResourceDesc.SampleDesc.Count = 1u;
			mFeedbackResourceDesc.SampleDesc.Quality = 0u;
			mFeedbackResourceDesc.SamplerFeedbackMipRegion.Height = GetTileTexelHeight();
			mFeedbackResourceDesc.SamplerFeedbackMipRegion.Width = GetTileTexelWidth();
			mFeedbackResourceDesc.SamplerFeedbackMipRegion.Depth = GetTileTexelDepth();

			const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			// 以默认方式创建Feedback
			HRASSERT(mDevice->D3DDevice()->CreateCommittedResource2(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&mFeedbackResourceDesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				nullptr,
				IID_PPV_ARGS(&mFeedbackTexture)
			));
		}

		// CPU heap used for ClearUnorderedAccessView on feedback map
		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = 1; // only need the one for the single feedback map
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			HRASSERT(mDevice->D3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mClearCpuUavHeap)));
		}

		// now that both feedback map and paired texture have been created,
		// can create the sampler feedback view
		{
			mDevice->D3DDevice()->CreateSamplerFeedbackUnorderedAccessView(
				mInternalTexture->D3DResource(),
				mFeedbackTexture.Get(),
				mClearCpuUavHeap->GetCPUDescriptorHandleForHeapStart());
			mFeedbackUADescriptor = mMainPoolDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			mDevice->D3DDevice()->CopyDescriptorsSimple(1u, mFeedbackUADescriptor->GetCpuHandle(),
				mClearCpuUavHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// create gpu-side resolve destination
		{
			TextureDesc _ResolvedTextureDesc{};

		}

		// create read back buffers
		{

		}
	}

	StreamVirtualTexture::~StreamVirtualTexture() {

	}

}