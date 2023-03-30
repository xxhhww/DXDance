#include "MemoryAliasingHelper.h"
#include "RenderGraphResourceStorage.h"
#include <algorithm>

namespace Renderer {

	MemoryAliasingHelper::MemoryAliasingHelper(RenderGraphResourceStorage* storage)
	: mResourceStorage(storage) {}

	void MemoryAliasingHelper::AddResource(RenderGraphResource* resource) {
		mNonAliasedResources.emplace(resource);
	}

	size_t MemoryAliasingHelper::BuildAliasing() {
		size_t optimalHeapSize{ 0u };

		while (!mNonAliasedResources.empty()) {
			auto it = mNonAliasedResources.begin();
			mCurrBucketAvailableSize = (*it)->GetRequiredMemory();
			optimalHeapSize += mCurrBucketAvailableSize;

			while (it != mNonAliasedResources.end()) {
				AliasWithAlreadyAliasedAllocations(*it);
				++it;
			}
			
			RemoveAliasedAllocations();

			mCurrBucketHeapOffset += mCurrBucketAvailableSize;
		}

		return optimalHeapSize;
	}

	void MemoryAliasingHelper::AliasWithAlreadyAliasedAllocations(RenderGraphResource* resource) {
		if (mAlreadyAliasedResources.empty() && resource->GetRequiredMemory() <= mCurrBucketAvailableSize) {
			resource->heapOffset = mCurrBucketHeapOffset;
			resource->aliased = true;
			mAlreadyAliasedResources.push_back(resource);
			return;
		}

		if (resource->GetRequiredMemory() > mCurrBucketAvailableSize) {
			return;
		}

		// Find memory regions in which we can place the next allocation based on previously found unavailable regions.
		// Pick the most fitting region. If next allocation cannot be fit in any free region, skip it.
		uint64_t nextAllocationSize = resource->GetRequiredMemory();
		MemoryRegion mostFittingMemoryRegion{ 0, 0 };
		int64_t overlapCounter = 0;

		for (auto i = 0u; i < mNonAliasableMemoryOffsets.size() - 1; ++i) {
			const auto& [currentOffset, currentType] = mNonAliasableMemoryOffsets[i];
			const auto& [nextOffset, nextType] = mNonAliasableMemoryOffsets[i + 1];

			overlapCounter += currentType == MemoryOffsetType::Start ? 1 : -1;
			overlapCounter = std::max(overlapCounter, 0ll);

			bool reachedAliasableRegion =
				overlapCounter == 0 &&
				currentType == MemoryOffsetType::End &&
				nextType == MemoryOffsetType::Start;

			if (reachedAliasableRegion) {
				MemoryRegion nextAliasableMemoryRegion{ currentOffset, nextOffset - currentOffset };
				FitAliasableMemoryRegion(nextAliasableMemoryRegion, nextAllocationSize, mostFittingMemoryRegion);
			}
		}

		// If we found a fitting aliasable memory region update allocation with an offset
		if (mostFittingMemoryRegion.Size > 0) {
			// Offset calculations were made in a frame relative to the current memory bucket.
			// Now we need to adjust it to be relative to the heap start.
			resource->heapOffset = mCurrBucketHeapOffset + mostFittingMemoryRegion.Offset;

			/*
			PipelineResourceSchedulingInfo::PassInfo* firstPassInfo = GetFirstPassInfo(nextSchedulingInfoIt);
			firstPassInfo->NeedsAliasingBarrier = true;

			// We aliased something with the first resource in the current memory bucket
			// so it's no longer a single occupant of this memory region, therefore it now
			// needs an aliasing barrier. If the first resource is a single resource on this
			// memory region then this code branch will never be hit and we will avoid a barrier for it.
			firstPassInfo = GetFirstPassInfo(mAlreadyAliasedAllocations.front());
			firstPassInfo->NeedsAliasingBarrier = true;
			*/

			mAlreadyAliasedResources.push_back(resource);
		}
	}

	void MemoryAliasingHelper::BuildMemoryRegionForCurrentResource(RenderGraphResource* resource) {
		mNonAliasableMemoryOffsets.clear();
		mNonAliasableMemoryOffsets.push_back({ 0, MemoryOffsetType::End });

		// Find memory regions in which we can't place the next allocation, because their allocations
		// are used simultaneously with the next one by some render passed (timelines)
		for (auto* alreadyAliasedResource : mAlreadyAliasedResources) {
			if (TimelinesIntersect(alreadyAliasedResource, resource)) {

				// Heap offset stored in AliasingInfo.HeapOffset is relative to heap beginning, 
				// but the algorithm requires non-aliasable memory region to be 
				// relative to current global offset, which is an offset of the current memory bucket we're aliasing resources in,
				// therefore we have to subtract current global offset
				uint64_t startByteIndex = alreadyAliasedResource->heapOffset - mCurrBucketHeapOffset;
				uint64_t endByteIndex = startByteIndex + alreadyAliasedResource->GetRequiredMemory();

				mNonAliasableMemoryOffsets.push_back({ startByteIndex, MemoryOffsetType::Start });
				mNonAliasableMemoryOffsets.push_back({ endByteIndex, MemoryOffsetType::End });
			}
		}

		mNonAliasableMemoryOffsets.push_back({ mCurrBucketAvailableSize, MemoryOffsetType::Start });

		std::sort(mNonAliasableMemoryOffsets.begin(), mNonAliasableMemoryOffsets.end(), 
		[](auto& offset1, auto& offset2) -> bool
		{
			return offset1.first < offset2.first;
		});

	}

	void MemoryAliasingHelper::RemoveAliasedAllocations() {
		for (auto* aliasedResource : mAlreadyAliasedResources) {
			mNonAliasedResources.erase(aliasedResource);
		}

		mAlreadyAliasedResources.clear();
	}

	bool MemoryAliasingHelper::TimelinesIntersect(const RenderGraphResource* a, const RenderGraphResource* b) const {
		return a->GetUsageTimeline().first <= b->GetUsageTimeline().second && b->GetUsageTimeline().first <= a->GetUsageTimeline().second;
	}

	void MemoryAliasingHelper::FitAliasableMemoryRegion(const MemoryRegion& nextAliasableRegion, uint64_t nextAllocationSize, MemoryRegion& optimalRegion) const {
		bool nextRegionValid = nextAliasableRegion.Size > 0;
		bool optimalRegionValid = optimalRegion.Size > 0;
		bool nextRegionIsMoreOptimal = nextAliasableRegion.Size <= optimalRegion.Size || !nextRegionValid || !optimalRegionValid;
		bool allocationFits = nextAllocationSize <= nextAliasableRegion.Size;

		if (allocationFits && nextRegionIsMoreOptimal) {
			optimalRegion.Offset = nextAliasableRegion.Offset;
			optimalRegion.Size = nextAllocationSize;
		}
	}

	bool MemoryAliasingHelper::Sort(RenderGraphResource* a, RenderGraphResource* b) {
		return a->GetRequiredMemory() > b->GetRequiredMemory();
	}

}