#include "StreamTexture.h"
#include "FileHandle.h"
#include "DataUploader.h"

#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"

#include "Tools/Assert.h"

namespace Renderer {

	StreamTexture::StreamTexture(
		const GHL::Device* device,
		DataUploader* dataUploader,
		const XeTexureFormat& xeTextureFormat,
		std::unique_ptr<FileHandle> fileHandle,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator,
		RingFrameTracker* frameTracker)
	: mDevice(device)
	, mDataUploader(dataUploader)
	, mFileFormat(xeTextureFormat)
	, mFileHandle(std::move(fileHandle))
	, mFrameTracker(frameTracker) 
	, mHeapAllocator(heapAllocator) 
	, mQueuedReadbackFeedback(mFrameTracker->GetMaxSize()) 
	, mPendingTileEvictions() {
		ResourceFormat resourceFormat{ mDevice, xeTextureFormat.ConvertTextureDesc() };
		ASSERT_FORMAT(resourceFormat.GetTextureDesc().supportStream == true, "SupportStream Is False");

		mInternalTexture = new Texture(device, resourceFormat, descriptorAllocator, heapAllocator);
		mPackedMipsFileOffset = mFileFormat.GetPackedMipFileOffset(&mPackedMipsNumBytes, &mPackedMipsUncompressedSize);
		
		const auto& d3dResourceDesc = resourceFormat.D3DResourceDesc();
		uint32_t subresourceCount = resourceFormat.SubresourceCount();
		mTiling.resize(subresourceCount);
		mDevice->D3DDevice()->GetResourceTiling(mInternalTexture->D3DResource(), &mNumTilesTotal, &mPackedMipInfo, &mTileShape, &subresourceCount, 0, &mTiling[0]);
		mNumStandardMips = mPackedMipInfo.NumStandardMips;
		mTileMappingState = std::make_unique<TileMappingState>(mNumStandardMips, mTiling);
		// 创建Feedback
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

		mMinMipMapCache.resize(GetNumTilesWidth()* GetNumTilesHeight(), mNumStandardMips);
		mMinMipMap.resize(mMinMipMapCache.size(), mNumStandardMips);
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
		const auto& currFrameAttribute = mFrameTracker->GetCurrFrameAttribute();
		mQueuedReadbackFeedback.at(currFrameAttribute.frameIndex).renderFrameFenceValue = currFrameAttribute.fenceValue;

		auto afterBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mResolvedResource.Get(), D3D12_RESOURCE_STATE_COPY_SOURCE, D3D12_RESOURCE_STATE_RESOLVE_DEST);
		commandList->ResourceBarrier(1u, &afterBarrier);
	}

	void StreamTexture::MapAndLoadPackedMipMap(GHL::CommandQueue* mappingQueue, GHL::Fence* packedMipMappingFence, IDStorageQueue* copyDsQueue, GHL::Fence* copyFence) {
		
		// 在Mapping之前，我们需要为PackedMipMap分配显存
		mPackedMipsHeapAllocation = mHeapAllocator->Allocate(mPackedMipsUncompressedSize);

		// 在数据复制之前，我们需要执行Mapping操作

		uint32_t firstSubresource = GetPackedMipInfo().NumStandardMips;

		// mapping packed mips is different from regular tiles: must be mapped before we can use copytextureregion() instead of copytiles()
		uint32_t numTiles = GetPackedMipInfo().NumTilesForPackedMips;

		std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags(numTiles, D3D12_TILE_RANGE_FLAG_NONE);

		// if the number of standard (not packed) mips is n, then start updating at subresource n
		D3D12_TILED_RESOURCE_COORDINATE resourceRegionStartCoordinates{ 0, 0, 0, firstSubresource };
		D3D12_TILE_REGION_SIZE resourceRegionSizes{ numTiles, FALSE, 0, 0, 0 };
		
		// perform packed mip tile mapping on the copy queue
		mappingQueue->D3DCommandQueue()->UpdateTileMappings(
			mInternalTexture->D3DResource(),
			numTiles,
			&resourceRegionStartCoordinates,
			&resourceRegionSizes,
			mPackedMipsHeapAllocation->heap->D3DHeap(),
			numTiles,
			rangeFlags.data(),
			&mPackedMipsHeapAllocation->tileOffset,
			nullptr,
			D3D12_TILE_MAPPING_FLAG_NONE
		);
		packedMipMappingFence->IncrementExpectedValue();
		mappingQueue->SignalFence(*packedMipMappingFence);
		packedMipMappingFence->Wait();

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

	void StreamTexture::ProcessReadbackFeedback() {
		mPendingTileEvictions.MoveToNextFrame();

		// 寻找最新鲜的Feedback
		bool feedbackFound = false;
		uint64_t latestFeedbackFenceValue = 0;
		uint32_t targetFeedbackIndex{ 0u };
		for (size_t i = 0; i < mQueuedReadbackFeedback.size(); i++) {
			if (mQueuedReadbackFeedback.at(i).isFresh) {
				uint64_t feedbackFenceValue = mQueuedReadbackFeedback.at(i).renderFrameFenceValue;
				if ((!feedbackFound) || (latestFeedbackFenceValue <= feedbackFenceValue)) {
					feedbackFound = true;
					targetFeedbackIndex = i;
					latestFeedbackFenceValue = feedbackFenceValue;
					// this feedback will either be used or skipped. either way it is "consumed"
					mQueuedReadbackFeedback.at(i).isFresh = false;
				}
			}
		}

		if (!feedbackFound) return;

		{
			uint8_t* pResolvedData{ nullptr };
			ID3D12Resource* pReadbackFeedbackResource = mReadbackResource.at(targetFeedbackIndex).Get();
			pReadbackFeedbackResource->Map(0u, nullptr, reinterpret_cast<void**>(&pResolvedData));


			uint32_t height = GetNumTilesHeight();
			uint32_t width = GetNumTilesWidth();

			uint8_t* pTileRow = mMinMipMapCache.data();
			for (uint32_t y = 0; y < height; y++) {
				for (uint32_t x = 0; x < width; x++) {
					// clamp to the maximum we are tracking (not tracking packed mips)
					uint8_t desired = std::min(pResolvedData[x], mNumStandardMips);
					uint8_t initialValue = pTileRow[x];
					SetMinMip(initialValue, x, y, desired);
					pTileRow[x] = desired;
				} // end loop over x
				pTileRow += width;
				pResolvedData += (width + 0x0ff) & ~0x0ff;

			} // end loop over y

			D3D12_RANGE emptyRange{ 0u, 0u };
			pReadbackFeedbackResource->Unmap(0u, &emptyRange);

			// 更新pendingEvictions
			mPendingTileEvictions.Rescue(*mTileMappingState);
		}
	}

	bool StreamTexture::ProcessTileLoadings() {
		// 将TileLoading任务推入DataUploader中
		if (mPendingTileLoadings.empty()) {
			return false;
		}

		UploadList* uploadList = mDataUploader->AllocateUploadList();
		uploadList->SetPendingStreamTexture(this);
		// 在显存堆上为每一个Tile进行分配
		uint32_t uploadSize{ 0u };
		for (const auto& coord : mPendingTileLoadings) {
			ASSERT_FORMAT(mTileMappingState->GetHeapAllocation(coord.X, coord.Y, coord.Subresource) == nullptr, "Heap Allocation Must Be Nullptr");
			
			if (mTileMappingState->GetResidencyState(coord.X, coord.Y, coord.Subresource) == TileMappingState::ResidencyState::NotResident) {
				BuddyHeapAllocator::Allocation* heapAllocation = mHeapAllocator->Allocate(mTileSize);

				mTileMappingState->SetHeapAllocation(coord.X, coord.Y, coord.Subresource, heapAllocation);
				mTileMappingState->SetResidencyState(coord.X, coord.Y, coord.Subresource, TileMappingState::ResidencyState::Loading);

				uploadList->PushPendingLoadings(coord, heapAllocation);
				uploadSize++;
			}
			else if (mTileMappingState->GetResidencyState(coord.X, coord.Y, coord.Subresource) == TileMappingState::ResidencyState::Evicting) {
				// 该Tile处于Evicting状态中
			}
		}
		mPendingTileLoadings.clear();
		mDataUploader->SubmitUploadList(uploadList);

		return false;
	}

	bool StreamTexture::ProcessTileEvictions() {
		if (mPendingTileEvictions.GetReadyToEvict().empty()) {
			return false;
		}

		auto& readyEvictions = mPendingTileEvictions.GetReadyToEvict();
		uint32_t numDelayed = 0u;
		for (const auto& coord : readyEvictions) {
			ASSERT_FORMAT(mTileMappingState->GetHeapAllocation(coord.X, coord.Y, coord.Subresource) != nullptr, "Heap Allocation Must Not Be Nullptr");

			if (mTileMappingState->GetResidencyState(coord.X, coord.Y, coord.Subresource) == TileMappingState::ResidencyState::Resident) {

				mHeapAllocator->Deallocate(mTileMappingState->GetHeapAllocation(coord.X, coord.Y, coord.Subresource));
				mTileMappingState->SetHeapAllocation(coord.X, coord.Y, coord.Subresource, nullptr);
				mTileMappingState->SetResidencyState(coord.X, coord.Y, coord.Subresource, TileMappingState::ResidencyState::NotResident);
			}
			else if (mTileMappingState->GetResidencyState(coord.X, coord.Y, coord.Subresource) == TileMappingState::ResidencyState::Loading) {
				
				// 需要Eviction的Tile正在Loading，保留该Evictions下次循环再来检测
				readyEvictions[numDelayed] = coord;
				numDelayed++;
			}
		}

		readyEvictions.resize(numDelayed);
		return numDelayed == 0u ? false : true;
	}

	void StreamTexture::FrameCompletedCallback(uint8_t frameIndex) {
		mQueuedReadbackFeedback.at(frameIndex).isFresh = true;
	}

	void StreamTexture::TileLoadingsCompletedCallback(const std::vector<D3D12_TILED_RESOURCE_COORDINATE>& coords) {
		for (const auto& coord : coords) {
			ASSERT_FORMAT(mTileMappingState->GetResidencyState(coord.X, coord.Y, coord.Subresource) == TileMappingState::ResidencyState::Loading,
				"Unsupported ResidencyState");
			mTileMappingState->SetResidencyState(coord.X, coord.Y, coord.Subresource, TileMappingState::ResidencyState::Resident);
		}

		// TODO 修改本地与全局的ResidencyMinMip
	}

	void StreamTexture::SetResidencyMapOffset(uint64_t mapOffset) {
		mResidencyMapOffset = mapOffset;
	}

	bool StreamTexture::IsStale() {
		return (mPendingTileLoadings.size() || mPendingTileEvictions.GetReadyToEvict().size());
	}

	void StreamTexture::SetMinMip(uint8_t currentMip, uint32_t x, uint32_t y, uint8_t desiredMip) {
		// what mip level is currently referenced at this tile?
		UINT8 s = currentMip;

		// addref mips we want
		// AddRef()s are ordered from bottom mip to top (all dependencies will be loaded first)
		while (s > desiredMip) {
			s -= 1; // already have "this" tile. e.g. have s == 1, desired in_s == 0, start with 0.
			AddTileRef(x >> s, y >> s, s);
		}

		// decref mips we don't need
		// DecRef()s are ordered from top mip to bottom (evict lower resolution tiles after all higher resolution ones)
		while (s < desiredMip) {
			DecTileRef(x >> s, y >> s, s);
			s++;
		}
	}

	void StreamTexture::AddTileRef(uint32_t x, uint32_t y, uint32_t s) {
		const auto refCount = mTileMappingState->GetRefCount(x, y, s);

		// if refcount is 0xffff... then adding to it will wrap around. shouldn't happen.
		ASSERT_FORMAT(~refCount);

		// need to allocate?
		if (0 == refCount) {
			mPendingTileLoadings.push_back(D3D12_TILED_RESOURCE_COORDINATE{ x, y, 0, s });
		}
		mTileMappingState->IncRefCount(x, y, s);
	}

	void StreamTexture::DecTileRef(uint32_t x, uint32_t y, uint32_t s) {
		const auto refCount = mTileMappingState->GetRefCount(x, y, s);

		ASSERT_FORMAT(0 != refCount);

		// last refrence? try to evict
		if (1 == refCount) {
			// queue up a decmapping request that will release the heap index after mapping and clear the resident flag
			mPendingTileEvictions.Append(D3D12_TILED_RESOURCE_COORDINATE{ x, y, 0, s });
		}
		mTileMappingState->DecRefCount(x, y, s);
	}

	// =============================== TileMappingState ===============================
	StreamTexture::TileMappingState::TileMappingState(uint32_t mipNums, std::vector<D3D12_SUBRESOURCE_TILING>& subresourceTilings) {
		mRefCounts.resize(mipNums);
		mResidencyStates.resize(mipNums);
		mHeapAllocations.resize(mipNums);

		for (uint32_t i = 0; i < subresourceTilings.size(); i++) {
			auto& tiling = subresourceTilings.at(i);
			if (tiling.WidthInTiles  == 0u 
				&& tiling.HeightInTiles == 0u 
				&& tiling.DepthInTiles  == 0u) continue;

			mRefCounts.at(i).resize(tiling.HeightInTiles);
			mResidencyStates.at(i).resize(tiling.HeightInTiles);
			mHeapAllocations.at(i).resize(tiling.HeightInTiles);

			for (uint32_t j = 0; j < tiling.HeightInTiles; j++) {
				mRefCounts.at(i).at(j).resize(tiling.WidthInTiles, 0u);
				mResidencyStates.at(i).at(j).resize(tiling.WidthInTiles, TileMappingState::ResidencyState::NotResident);
				mHeapAllocations.at(i).at(j).resize(tiling.WidthInTiles, nullptr);
			}
		}
	}

	StreamTexture::TileMappingState::~TileMappingState() {
	}

	void StreamTexture::TileMappingState::SetRefCount(uint32_t x, uint32_t y, uint32_t s, uint32_t refCount) {
		mRefCounts[s][y][x] = refCount;
	}
	
	void StreamTexture::TileMappingState::IncRefCount(uint32_t x, uint32_t y, uint32_t s) {
		mRefCounts[s][y][x] ++;
	}

	void StreamTexture::TileMappingState::DecRefCount(uint32_t x, uint32_t y, uint32_t s) {
		mRefCounts[s][y][x] --;
	}

	void StreamTexture::TileMappingState::SetResidencyState(uint32_t x, uint32_t y, uint32_t s, ResidencyState state) {
		mResidencyStates[s][y][x] = state;
	}

	void StreamTexture::TileMappingState::SetHeapAllocation(uint32_t x, uint32_t y, uint32_t s, BuddyHeapAllocator::Allocation* heapAllocation) {
		mHeapAllocations[s][y][x] = heapAllocation;
	}

	// =============================== Eviction ===============================
	StreamTexture::EvictionDelay::EvictionDelay(uint32_t nFrames) {
		mEvictionsBuffer.resize(nFrames);
	}

	void StreamTexture::EvictionDelay::MoveToNextFrame() {
		// start with A, B, C
		// after swaps, have C, A, B
		// then insert C, A, BC
		// then clear 0, A, BC
		uint32_t lastMapping = (uint32_t)mEvictionsBuffer.size() - 1;
		for (uint32_t i = lastMapping; i > 0; i--)
		{
			mEvictionsBuffer[i].swap(mEvictionsBuffer[i - 1]);
		}
		mEvictionsBuffer.back().insert(mEvictionsBuffer.back().end(), mEvictionsBuffer[0].begin(), mEvictionsBuffer[0].end());
		mEvictionsBuffer[0].clear();
	}

	void StreamTexture::EvictionDelay::Clear() {
		for (auto& i : mEvictionsBuffer) {
			i.clear();
		}
	}

	void StreamTexture::EvictionDelay::Rescue(const TileMappingState& in_tileMappingState) {
		// note: it is possible even for the most recent evictions to have refcount > 0
		// because a tile can be evicted then loaded again within a single ProcessFeedback() call
		for (auto& evictions : mEvictionsBuffer) {
			uint32_t numPending = (uint32_t)evictions.size();
			for (uint32_t i = 0; i < numPending;) {
				auto& c = evictions[i];
				// on rescue, swap a later tile in and re-try the check
				// this re-orders the queue, but we can tolerate that
				// because the residency map is built bottom-up
				if (in_tileMappingState.GetRefCount(c.X, c.Y, c.Subresource)) {
					numPending--;
					c = evictions[numPending];
				}
				else { // refcount still 0, this tile may still be evicted 
					i++;
				}
			}
			evictions.resize(numPending);
		}
	}

}