#include "Renderer/StreamVirtualTexture.h"
#include "Renderer/TileUploader.h"

namespace Renderer {

	StreamVirtualTexture::StreamVirtualTexture(
		const GHL::Device* device,
		TileUploader* tileUploader,
		RingFrameTracker* mainRingFrameTracker,
		ResourceAllocator* mainResourceAllocator,
		BuddyHeapAllocator* mainBuddyheapAllocator,
		ResourceStateTracker* mainResourceStateTracker,
		PoolDescriptorAllocator* mainPoolDescriptorAllocator,
		const XeTexureFormat& fileFormat,
		std::unique_ptr<FileHandle>&& fileHandle)
	: mDevice(device)
	, mTileUploader(tileUploader)
	, mMainRingFrameTracker(mainRingFrameTracker)
	, mMainResourceAllocator(mainResourceAllocator)
	, mMainBuddyHeapAllocator(mainBuddyheapAllocator)
	, mMainResourceStateTracker(mainResourceStateTracker)
	, mMainPoolDescriptorAllocator(mainPoolDescriptorAllocator)
	, mFileFormat(fileFormat)
	, mFileHandle(std::move(fileHandle)) {

		ResourceFormat resourceFormat{ mDevice, mFileFormat.ConvertTextureDesc() };
		auto& textureDesc = resourceFormat.GetTextureDesc();
		ASSERT_FORMAT(textureDesc.createdMethod == GHL::ECreatedMethod::Reserved, "StreamVirtualTexture Must Create Reserved");

		// 创建 InternalTexture并追踪
		mInternalTexture = mMainResourceAllocator->Allocate(device, textureDesc, mMainPoolDescriptorAllocator, mMainBuddyHeapAllocator);
		mInternalTexture->SetDebugName("SVT#" + mFileFormat.GetFilename());
		mMainResourceStateTracker->StartTracking(mInternalTexture);

		mPackedMipsFileOffset = mFileFormat.GetPackedMipFileOffset(&mPackedMipsNumBytes, &mPackedMipsUncompressedSize);

		const auto& d3dResourceDesc = resourceFormat.D3DResourceDesc();
		uint32_t subresourceCount = resourceFormat.SubresourceCount();
		mTiling.resize(subresourceCount);
		mDevice->D3DDevice()->GetResourceTiling(mInternalTexture->D3DResource(), &mNumTilesTotal, &mPackedMipInfo, &mTileShape, &subresourceCount, 0, &mTiling[0]);
		mNumStandardMips = mPackedMipInfo.NumStandardMips;
		mTileMappingState = std::make_unique<TileMappingState>(mNumStandardMips, mTiling);

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
			std::wstring feedbackName = Tool::StrUtil::UTF8ToWString("Feedback#" + mFileFormat.GetFilename());
			mFeedbackTexture->SetName(feedbackName.c_str());
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
			_ResolvedTextureDesc.width = GetTileTexelWidth();
			_ResolvedTextureDesc.height = GetTileTexelHeight();
			_ResolvedTextureDesc.format = DXGI_FORMAT_R8_UINT;
			_ResolvedTextureDesc.expectedState = GHL::EResourceState::ResolveDestination | GHL::EResourceState::CopySource;
			mResolvedTexture = mMainResourceAllocator->Allocate(mDevice, _ResolvedTextureDesc, mMainPoolDescriptorAllocator, nullptr);
			mResolvedTexture->SetDebugName("ResolveDest#" + mFileFormat.GetFilename());
			mMainResourceStateTracker->StartTracking(mResolvedTexture);
		}

		// create read back buffers
		{
			BufferDesc _ReadbackDesc{};
			_ReadbackDesc.size = ((GetTileTexelWidth() + 0x0ff) & ~0x0ff) * GetTileTexelHeight();	// 256字节对齐
			_ReadbackDesc.usage = GHL::EResourceUsage::ReadBack;
			_ReadbackDesc.initialState = GHL::EResourceState::CopyDestination;
			_ReadbackDesc.expectedState = _ReadbackDesc.initialState;
			mReadbackBuffers.resize(mMainRingFrameTracker->GetMaxSize());
			for (uint32_t i = 0; i < mReadbackBuffers.size(); i++) {
				mReadbackBuffers[i] = mMainResourceAllocator->Allocate(
					device, _ReadbackDesc, mMainPoolDescriptorAllocator, nullptr
				);
			}
		}

		mMinMipMapCache.resize(GetNumTilesWidth()* GetNumTilesHeight(), mNumStandardMips);
		mMinMipMap.resize(mMinMipMapCache.size(), mNumStandardMips);
	}

	StreamVirtualTexture::~StreamVirtualTexture() {

	}

	void StreamVirtualTexture::ProcessFeedback(uint64_t completedFenceValue) {
		mPendingTileEvictions.MoveToNextFrame();

		// 寻找最新鲜的Feedback
		bool feedbackFound = false;
		uint64_t latestFeedbackFenceValue = 0;
		uint32_t targetFeedbackIndex{ 0u };
		for (size_t i = 0; i < mQueuedReadbacks.size(); i++) {
			if (mQueuedReadbacks.at(i).isFresh) {
				uint64_t feedbackFenceValue = mQueuedReadbacks.at(i).renderFrameFenceValue;
				if ((!feedbackFound) || (latestFeedbackFenceValue <= feedbackFenceValue)) {
					feedbackFound = true;
					targetFeedbackIndex = i;
					latestFeedbackFenceValue = feedbackFenceValue;
					// this feedback will either be used or skipped. either way it is "consumed"
					mQueuedReadbacks.at(i).isFresh = false;
				}
			}
		}

		if (!feedbackFound) {
			return;
		}

		{
			BufferWrap& readbackBuffer = mReadbackBuffers.at(targetFeedbackIndex);
			uint8_t* pResolvedData = readbackBuffer->Map();

			uint32_t height = GetNumTilesHeight();
			uint32_t width = GetNumTilesWidth();

			uint8_t* pTileRow = mMinMipMapCache.data();
			for (uint32_t y = 0; y < height; y++) {
				for (uint32_t x = 0; x < width; x++) {
					// clamp to the maximum we are tracking (not tracking packed mips)
					uint8_t desiredMip = std::min(pResolvedData[x], mNumStandardMips);
					uint8_t currentMip = pTileRow[x];
					SetMinMip(currentMip, x, y, desiredMip);
					pTileRow[x] = desiredMip;
				}	// end loop over x
				pTileRow += width;
				pResolvedData += (width + 0x0ff) & ~0x0ff;

			} // end loop over y

			readbackBuffer->UnMap();

			// abandon pending loads that are no longer relevant
			RescanPendingTileLoads();

			// 更新pendingEvictions
			mPendingTileEvictions.Rescue(*mTileMappingState);
		}
	}

	uint32_t StreamVirtualTexture::ProcessTileLoadings() {
		if (mPendingTileLoadings.empty()) {
			return 0;
		}

		TileUploader::Task* task = mTileUploader->AllocateTask();

		uint32_t numProcessedTiles = 0u;
		for (auto& pendingTileCoord : mPendingTileLoadings) {
			if (mTileMappingState->GetResidencyState(pendingTileCoord.X, pendingTileCoord.Y, pendingTileCoord.Subresource) == TileMappingState::ResidencyState::NotResident) {
				BuddyHeapAllocator::Allocation* heapAllocation = mMainBuddyHeapAllocator->Allocate(smTileSize);

				mTileMappingState->SetHeapAllocation(pendingTileCoord.X, pendingTileCoord.Y, pendingTileCoord.Subresource, heapAllocation);
				mTileMappingState->SetResidencyState(pendingTileCoord.X, pendingTileCoord.Y, pendingTileCoord.Subresource, TileMappingState::ResidencyState::Loading);

				task->PushPendingLoadings(pendingTileCoord, heapAllocation);

				numProcessedTiles++;
			}
			// drop Loading State / Resident State
		}
		mPendingTileLoadings.clear();
		mTileUploader->SubmitTask(task);

		return numProcessedTiles;
	}

	uint32_t StreamVirtualTexture::ProcessTileEvictions() {
		if (mPendingTileEvictions.GetReadyToEvict().empty()) { 
			return 0; 
		}

		auto& pendingEvictions = mPendingTileEvictions.GetReadyToEvict();

		UINT numDelayed = 0;
		UINT numEvictions = 0;
		for (auto& coord : pendingEvictions) {
			// if the heap index is valid, but the tile is not resident, there's a /pending load/
			// a pending load might be streaming OR it might be in the pending list
			// if in the pending list, we will observe if the refcount is 0 and abandon the load

			// NOTE! assumes refcount is 0
			// ProcessFeedback() clears all pending evictions with refcount > 0
			// Hence, ProcessFeedback() must be called before this function
			ASSERT_FORMAT(0 == mTileMappingState->GetRefCount(coord.X, coord.Y, coord.Subresource), "");

			auto& residencyState = mTileMappingState->GetResidencyState(coord.X, coord.Y, coord.Subresource);
			if (TileMappingState::ResidencyState::Resident == residencyState)
			{
				// NOTE: effectively removed "Evicting." Now remove tiles from data structure, not from memory mapping.
				// result is improved perf from fewer UpdateTileMappings() calls.
				// existing artifacts (cracks when sampler crosses tile boundaries) are "no worse"
				// to put it back: set residency to evicting and add tiles to updatelist for eviction
				mMainBuddyHeapAllocator->Deallocate(mTileMappingState->GetHeapAllocation(coord.X, coord.Y, coord.Subresource));
				mTileMappingState->SetHeapAllocation(coord.X, coord.Y, coord.Subresource, nullptr);
				mTileMappingState->SetResidencyState(coord.X, coord.Y, coord.Subresource, TileMappingState::ResidencyState::NotResident);

				numEvictions++;
			}
			// valid index but not resident means there is a pending load, do not evict
			// try again later
			else if (TileMappingState::ResidencyState::Loading == residencyState)
			{
				pendingEvictions[numDelayed] = coord;
				numDelayed++;
			}
			// if evicting or not resident, drop

			// else: refcount positive or eviction already in progress? rescue this eviction (by not adding to pending evictions)
		}

		if (numEvictions) {
			SetResidencyChanged();
		}

		// replace the ready evictions with just the delayed evictions.
		pendingEvictions.resize(numDelayed);
		return numEvictions;
	}

	void StreamVirtualTexture::OnCopyCompleted(std::vector<D3D12_TILED_RESOURCE_COORDINATE>& coords) {
		for (const auto& coord : coords) {
			ASSERT_FORMAT(mTileMappingState->GetResidencyState(coord.X, coord.Y, coord.Subresource) == TileMappingState::ResidencyState::Loading,
				"ResidencyState Must be Loading");
			mTileMappingState->SetResidencyState(coord.X, coord.Y, coord.Subresource, TileMappingState::ResidencyState::Resident);
		}

		SetResidencyChanged();
	}

	void StreamVirtualTexture::RecordClearFeedback(CommandBuffer& commandBuffer) {
		UINT clearValue[4]{};
		commandBuffer.D3DCommandList()->ClearUnorderedAccessViewUint(
			mFeedbackUADescriptor.Get()->GetGpuHandle(),
			mClearCpuUavHeap->GetCPUDescriptorHandleForHeapStart(),
			mFeedbackTexture.Get(),
			clearValue,
			0,
			nullptr);
	}

	void StreamVirtualTexture::RecordResolve(CommandBuffer& commandBuffer) {
		auto beforeBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mFeedbackTexture.Get(), D3D12_RESOURCE_STATE_UNORDERED_ACCESS, D3D12_RESOURCE_STATE_RESOLVE_SOURCE);
		commandBuffer.D3DCommandList()->ResourceBarrier(1u, &beforeBarrier);

		auto barrierBatch = commandBuffer.TransitionImmediately(mResolvedTexture, GHL::EResourceState::ResolveDestination);
		commandBuffer.FlushResourceBarrier(barrierBatch);

		auto* resolveDest = mResolvedTexture->D3DResource();

		// resolve the min mip map
		// can resolve directly to a host readable buffer
		commandBuffer.D3DCommandList()->ResolveSubresourceRegion(
			resolveDest,
			0,                   // decode target only has 1 layer (or is a buffer)
			0, 0,
			mFeedbackTexture.Get(),
			UINT_MAX,            // decode SrcSubresource must be UINT_MAX
			nullptr,             // src rect is not supported for min mip maps
			DXGI_FORMAT_R8_UINT, // decode format must be R8_UINT
			D3D12_RESOLVE_MODE_DECODE_SAMPLER_FEEDBACK
		);

		auto afterBarrier = CD3DX12_RESOURCE_BARRIER::Transition(mFeedbackTexture.Get(), D3D12_RESOURCE_STATE_RESOLVE_SOURCE, D3D12_RESOURCE_STATE_UNORDERED_ACCESS);
		commandBuffer.D3DCommandList()->ResourceBarrier(1u, &afterBarrier);
	}

	void StreamVirtualTexture::RecordReadback(CommandBuffer& commandBuffer) {
		auto barrierBatch = commandBuffer.TransitionImmediately(mResolvedTexture, GHL::EResourceState::CopySource);
		commandBuffer.FlushResourceBarrier(barrierBatch);

		uint8_t currentFrameIndex = mMainRingFrameTracker->GetCurrFrameIndex();
		BufferWrap& resolvedReadback = mReadbackBuffers[currentFrameIndex];
		auto& srcDesc = mResolvedTexture->GetResourceFormat().D3DResourceDesc();

		uint32_t rowPitch = (srcDesc.Width * GHL::GetFormatStride(srcDesc.Format) + 0x0ff) & ~0x0ff;
		D3D12_PLACED_SUBRESOURCE_FOOTPRINT layout{ 0,
			{ srcDesc.Format, (UINT)srcDesc.Width, srcDesc.Height, srcDesc.DepthOrArraySize, rowPitch } };

		D3D12_TEXTURE_COPY_LOCATION srcLocation = CD3DX12_TEXTURE_COPY_LOCATION(mResolvedTexture->D3DResource(), 0);
		D3D12_TEXTURE_COPY_LOCATION dstLocation = CD3DX12_TEXTURE_COPY_LOCATION(resolvedReadback->D3DResource(), layout);

		commandBuffer.D3DCommandList()->CopyTextureRegion(
			&dstLocation,
			0, 0, 0,
			&srcLocation,
			nullptr);
		const auto& currFrameAttribute = mMainRingFrameTracker->GetCurrFrameAttribute();
		mQueuedReadbacks.at(currFrameAttribute.frameIndex).renderFrameFenceValue = currFrameAttribute.fenceValue;
	}

	void StreamVirtualTexture::SetMinMip(uint8_t currentMip, uint32_t x, uint32_t y, uint8_t desiredMip) {
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

	void StreamVirtualTexture::AddTileRef(uint32_t x, uint32_t y, uint32_t s) {
		const auto refCount = mTileMappingState->GetRefCount(x, y, s);

		// if refcount is 0xffff... then adding to it will wrap around. shouldn't happen.
		ASSERT_FORMAT(~refCount);

		// need to allocate?
		if (0 == refCount) {
			mPendingTileLoadings.push_back(D3D12_TILED_RESOURCE_COORDINATE{ x, y, 0, s });
		}
		mTileMappingState->IncRefCount(x, y, s);
	}

	void StreamVirtualTexture::DecTileRef(uint32_t x, uint32_t y, uint32_t s) {
		const auto refCount = mTileMappingState->GetRefCount(x, y, s);

		ASSERT_FORMAT(0 != refCount);

		// last refrence? try to evict
		if (1 == refCount) {
			// queue up a decmapping request that will release the heap index after mapping and clear the resident flag
			mPendingTileEvictions.Append(D3D12_TILED_RESOURCE_COORDINATE{ x, y, 0, s });
		}
		mTileMappingState->DecRefCount(x, y, s);
	}

	void StreamVirtualTexture::RescanPendingTileLoads() {
		uint32_t numPending = (uint32_t)mPendingTileLoadings.size();

		for (uint32_t i = 0; i < numPending;) {
			auto& pending = mPendingTileLoadings.at(i);

			if (mTileMappingState->GetRefCount(pending.X, pending.Y, pending.Subresource)) {
				i++;
			}
			else {
				numPending--;
				pending = mPendingTileLoadings.at(numPending);
			}
		}
		mPendingTileLoadings.resize(numPending);
	}

	void StreamVirtualTexture::SetResidencyChanged() {

	}

	void StreamVirtualTexture::UpdateMinMipMap() {

	}

	StreamVirtualTexture::TileMappingState::TileMappingState(uint32_t mipNums, std::vector<D3D12_SUBRESOURCE_TILING>& subresourceTilings) {
		mRefCounts.resize(mipNums);
		mResidencyStates.resize(mipNums);
		mHeapAllocations.resize(mipNums);

		for (uint32_t i = 0; i < subresourceTilings.size(); i++) {
			auto& tiling = subresourceTilings.at(i);
			if (tiling.WidthInTiles == 0u
				&& tiling.HeightInTiles == 0u
				&& tiling.DepthInTiles == 0u) continue;

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
	
	StreamVirtualTexture::TileMappingState::~TileMappingState() {
	}

	StreamVirtualTexture::EvictionDelay::EvictionDelay(uint32_t nFrames) {
		mEvictionsBuffer.resize(nFrames);
	}

	StreamVirtualTexture::EvictionDelay::~EvictionDelay() {
	}

	void StreamVirtualTexture::EvictionDelay::MoveToNextFrame() {
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

	void StreamVirtualTexture::EvictionDelay::Clear() {
		for (auto& i : mEvictionsBuffer) {
			i.clear();
		}
	}

	void StreamVirtualTexture::EvictionDelay::Rescue(const TileMappingState& in_tileMappingState) {
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