#include "StreamTexture.h"
#include "FileHandle.h"

#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"

#include "Tools/Assert.h"

namespace Renderer {

	StreamTexture::StreamTexture(
		const GHL::Device* device,
		const XeTexureFormat& xeTextureFormat,
		std::unique_ptr<FileHandle> fileHandle,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator,
		RingFrameTracker* frameTracker)
	: mDevice(device)
	, mFileFormat(xeTextureFormat)
	, mFileHandle(std::move(fileHandle))
	, mFrameTracker(frameTracker) 
	, mHeapAllocator(heapAllocator) {
		ResourceFormat resourceFormat{ mDevice, xeTextureFormat.ConvertTextureDesc() };
		ASSERT_FORMAT(resourceFormat.GetTextureDesc().supportStream == true, "SupportStream Is False");

		mInternalTexture = new Texture(device, resourceFormat, descriptorAllocator, heapAllocator);
		mMaxMip = resourceFormat.SubresourceCount();
		mPackedMipsFileOffset = mFileFormat.GetPackedMipFileOffset(&mPackedMipsNumBytes, &mPackedMipsUncompressedSize);
		const auto& d3dResourceDesc = resourceFormat.D3DResourceDesc();

		mTiling.resize(mMaxMip);
		mDevice->D3DDevice()->GetResourceTiling(mInternalTexture->D3DResource(), &mNumTilesTotal, &mPackedMipInfo, &mTileShape, &mMaxMip, 0, &mTiling[0]);

		// 创建Feedback
		{
			mFeedbackResourceDesc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
			mFeedbackResourceDesc.Format = DXGI_FORMAT_SAMPLER_FEEDBACK_MIN_MIP_OPAQUE;
			mFeedbackResourceDesc.MipLevels = mMaxMip;
			mFeedbackResourceDesc.Alignment = 0u;
			mFeedbackResourceDesc.DepthOrArraySize = d3dResourceDesc.DepthOrArraySize;
			mFeedbackResourceDesc.Height = d3dResourceDesc.Height;
			mFeedbackResourceDesc.Width = d3dResourceDesc.Width;
			mFeedbackResourceDesc.Flags = D3D12_RESOURCE_FLAG_ALLOW_UNORDERED_ACCESS;
			mFeedbackResourceDesc.SampleDesc.Count = 1u;
			mFeedbackResourceDesc.SampleDesc.Quality = 0u;
			mFeedbackResourceDesc.SamplerFeedbackMipRegion.Height = GetTileTexelHeight();
			mFeedbackResourceDesc.SamplerFeedbackMipRegion.Width  = GetTileTexelWidth();
			mFeedbackResourceDesc.SamplerFeedbackMipRegion.Depth  = GetTileTexelDepth();

			const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			// 以默认方式创建Feedback
			HRASSERT(mDevice->D3DDevice()->CreateCommittedResource2(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&mFeedbackResourceDesc,
				D3D12_RESOURCE_STATE_UNORDERED_ACCESS,
				nullptr,
				nullptr,
				IID_PPV_ARGS(&mFeedbackResource)
			));
			mFeedbackResource->SetName(L"Feedback");
		}

		// CPU heap used for ClearUnorderedAccessView on feedback map
		{
			D3D12_DESCRIPTOR_HEAP_DESC desc = {};
			desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
			desc.NumDescriptors = 1; // only need the one for the single feedback map
			desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
			HRASSERT(mDevice->D3DDevice()->CreateDescriptorHeap(&desc, IID_PPV_ARGS(&mClearUavHeap)));
		}

		// now that both feedback map and paired texture have been created,
		// can create the sampler feedback view
		{
			mDevice->D3DDevice()->CreateSamplerFeedbackUnorderedAccessView(
				mInternalTexture->D3DResource(),
				mFeedbackResource.Get(),
				mClearUavHeap->GetCPUDescriptorHandleForHeapStart());
			mFeedbackUADescriptor = mInternalTexture->mDescriptorAllocator->Allocate(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
			mDevice->D3DDevice()->CopyDescriptorsSimple(1u, mFeedbackUADescriptor->GetCpuHandle(),
				mClearUavHeap->GetCPUDescriptorHandleForHeapStart(), D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
		}

		// create gpu-side resolve destination
		{
			D3D12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer(GetNumTilesWidth() * GetNumTilesHeight());
			const auto heapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT);

			const auto resolvedResourceDesc = CD3DX12_RESOURCE_DESC::Tex2D(DXGI_FORMAT_R8_UINT, GetNumTilesWidth(), GetNumTilesHeight(), 1, 1);

			HRASSERT(mDevice->D3DDevice()->CreateCommittedResource(
				&heapProperties,
				D3D12_HEAP_FLAG_NONE,
				&resolvedResourceDesc,
				// NOTE: though used as RESOLVE_DEST, it is also copied to the CPU
				// start in the copy_source state to align with transition barrier logic in TileUpdateManager
				D3D12_RESOURCE_STATE_RESOLVE_DEST,
				nullptr,
				IID_PPV_ARGS(&mResolvedResource)));
			mResolvedResource->SetName(L"ResolvedResource");
		}

		{
			D3D12_RESOURCE_DESC rd = CD3DX12_RESOURCE_DESC::Buffer(GetNumTilesWidth() * GetNumTilesHeight());
			const auto resolvedHeapProperties = CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_READBACK);

			// CopyTextureRegion requires pitch multiple of D3D12_TEXTURE_DATA_PITCH_ALIGNMENT = 256
			UINT pitch = GetNumTilesWidth();
			pitch = (pitch + 0x0ff) & ~0x0ff;
			rd.Width = pitch * GetNumTilesHeight();

			mReadbackResource.resize(mFrameTracker->GetMaxSize());

			int i = 0;
			for (auto& rb : mReadbackResource) {
				HRASSERT(mDevice->D3DDevice()->CreateCommittedResource(
					&resolvedHeapProperties,
					D3D12_HEAP_FLAG_NONE,
					&rd,
					// start in the copy_source state to align with transition barrier logic in TileUpdateManager
					D3D12_RESOURCE_STATE_COPY_DEST,
					nullptr,
					IID_PPV_ARGS(&rb)));
				rb->SetName(std::to_wstring(i).c_str());
				i++;
			}
		}
	}

	StreamTexture::~StreamTexture() {
		if (mInternalTexture != nullptr) {
			delete mInternalTexture;
		}
	}

	void StreamTexture::RecordClearFeedback(ID3D12GraphicsCommandList4* commandList) {
		UINT clearValue[4]{};
		commandList->ClearUnorderedAccessViewUint(
			mFeedbackUADescriptor.Get()->GetGpuHandle(),
			mClearUavHeap->GetCPUDescriptorHandleForHeapStart(),
			mFeedbackResource.Get(),
			clearValue,
			0, 
			nullptr);
	}

	void StreamTexture::RecordResolve(ID3D12GraphicsCommandList4* commandList) {
		auto beforeBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mFeedbackResource.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
		commandList->ResourceBarrier(1u, &beforeBarrier);

		auto resolveDest = mResolvedResource.Get();

		// resolve the min mip map
		// can resolve directly to a host readable buffer
		commandList->ResolveSubresourceRegion(
			resolveDest,
			0,                   // decode target only has 1 layer (or is a buffer)
			0, 0,
			mFeedbackResource.Get(),
			UINT_MAX,            // decode SrcSubresource must be UINT_MAX
			nullptr,             // src rect is not supported for min mip maps
			DXGI_FORMAT_R8_UINT, // decode format must be R8_UINT
			D3D12_RESOLVE_MODE_DECODE_SAMPLER_FEEDBACK
		);

		auto afterBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mFeedbackResource.Get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandList->ResourceBarrier(1u, &afterBarrier);
	}

	void StreamTexture::RecordReadback(ID3D12GraphicsCommandList4* commandList) {
		auto beforeBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mResolvedResource.Get(), D3D12_RESOURCE_STATE_RESOLVE_DEST, D3D12_RESOURCE_STATE_COPY_SOURCE);
		commandList->ResourceBarrier(1u, &beforeBarrier);

		uint8_t currentFrameIndex = mFrameTracker->GetCurrFrameIndex();
		ID3D12Resource* pResolvedReadback = mReadbackResource[currentFrameIndex].Get();
		auto srcDesc = mResolvedResource->GetDesc();
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{ 0,
			{srcDesc.Format, (UINT)srcDesc.Width, srcDesc.Height, 1, (UINT)srcDesc.Width } };
		layout.Footprint.RowPitch = (layout.Footprint.RowPitch + 0x0ff) & ~0x0ff;

		D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(mResolvedResource.Get(), 0);
		D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(pResolvedReadback, layout);

		commandList->CopyTextureRegion(
			&dstLocation,
			0, 0, 0,
			&srcLocation,
			nullptr);

		auto afterBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mResolvedResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);
		commandList->ResourceBarrier(1u, &afterBarrier);
	}

	void StreamTexture::MapAndLoadPackedMipMap(GHL::CommandQueue* mappingQueue, GHL::Fence* mappingFence, IDStorageQueue* copyDsQueue, GHL::Fence* copyFence) {
		
		// 在Mapping之前，我们需要为PackedMipMap分配显存
		mPackedMipsHeapAllocation = mHeapAllocator->Allocate(mPackedMipsUncompressedSize);

		// 在数据复制之前，我们需要执行Mapping操作

		UINT firstSubresource = GetPackedMipInfo().NumStandardMips;

		// mapping packed mips is different from regular tiles: must be mapped before we can use copytextureregion() instead of copytiles()
		UINT numTiles = GetPackedMipInfo().NumTilesForPackedMips;

		std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags(numTiles, D3D12_TILE_RANGE_FLAG_NONE);

		// if the number of standard (not packed) mips is n, then start updating at subresource n
		D3D12_TILED_RESOURCE_COORDINATE resourceRegionStartCoordinates{ 0, 0, 0, firstSubresource };
		D3D12_TILE_REGION_SIZE resourceRegionSizes{ numTiles, FALSE, 0, 0, 0 };

		// perform packed mip tile mapping on the copy queue
		mappingQueue->D3DCommandQueue()->UpdateTileMappings(
			mInternalTexture->D3DResource(),
			1, // numRegions
			&resourceRegionStartCoordinates,
			&resourceRegionSizes,
			mPackedMipsHeapAllocation->heap->D3DHeap(),
			numTiles,
			rangeFlags.data(),
			&mPackedMipsHeapAllocation->tileOffset,
			nullptr,
			D3D12_TILE_MAPPING_FLAG_NONE
		);
		mappingFence->IncrementExpectedValue();
		mappingQueue->SignalFence(*mappingFence);
		mappingFence->Wait();

		// 执行数据复制操作
		// 注意: PackedMip虽然只是一个Tile，但是跨越多个子资源，所以DestinationType设置为MULTIPLE_SUBRESOURCES
		DSTORAGE_REQUEST request{};
		request.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_MULTIPLE_SUBRESOURCES;
		request.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		request.Options.CompressionFormat = (DSTORAGE_COMPRESSION_FORMAT)mFileFormat.GetCompressionFormat();
		request.Source.File.Source = mFileHandle->GetDStorageFile();
		request.Source.File.Offset = mPackedMipsFileOffset;
		request.Source.File.Size = mPackedMipsNumBytes;
		request.Destination.MultipleSubresources.Resource = mInternalTexture->D3DResource();
		request.Destination.MultipleSubresources.FirstSubresource = firstSubresource;
		request.UncompressedSize = mPackedMipsUncompressedSize;

		copyDsQueue->EnqueueRequest(&request);
		copyFence->IncrementExpectedValue();
		copyDsQueue->EnqueueSignal(copyFence->D3DFence(), copyFence->ExpectedValue());
		copyDsQueue->Submit();

		copyFence->Wait();
	}

	void StreamTexture::SetResidencyMapOffset(uint64_t mapOffset) {
		mResidencyMapOffset = mapOffset;
	}

}