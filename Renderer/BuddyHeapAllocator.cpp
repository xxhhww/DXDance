#include "BuddyHeapAllocator.h"

namespace Renderer {
	BuddyHeapAllocator::BuddyHeapAllocator(const GHL::Device* device, RingFrameTracker* ringFrameTracker)
	: mDevice(device)
	, mFrameTracker(ringFrameTracker)
	, mMinBlockSize(mDevice->GetHeapAlignment())       // 最小块: 64KB
	, mMaxBlockSize(mDevice->GetHeapAlignment() * 512) // 最大快: 32MB = 64KB * 512
	, mBuddyHeapPool(mMinBlockSize, mMaxBlockSize) {
		mFrameTracker->AddFrameCompletedCallBack([this](const size_t& frameIndex) {
			CleanUpPendingDeallocation(frameIndex);
		});

		mPendingDeallocations.resize(mFrameTracker->GetMaxSize());
	}

	BuddyHeapAllocator::Allocation* BuddyHeapAllocator::Allocate(size_t memorySize) {
		auto* poolAllocation = mBuddyHeapPool.Allocate(memorySize);
		if (!poolAllocation->bucket->userData.heapIndex) {
			mHeaps.emplace_back(new GHL::Heap(mDevice, mMaxBlockSize, GHL::EResourceUsage::Default));

			poolAllocation->bucket->userData.heapIndex = mHeaps.size() - 1;
		}

		auto heapIndex = *poolAllocation->bucket->userData.heapIndex;
		auto heapOffset = poolAllocation->block->offset;
		auto tileOffset = heapOffset / mMinBlockSize;
		auto size = poolAllocation->block->size;
		auto* heap = mHeaps.at(heapIndex).get();

		return new Allocation(poolAllocation, heap, heapIndex, heapOffset, tileOffset, size);
	}

	void BuddyHeapAllocator::Deallocate(BuddyHeapAllocator::Allocation* allocation) {
		mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(allocation);
	}


	void BuddyHeapAllocator::CleanUpPendingDeallocation(uint8_t frameIndex) {
		for (size_t i = 0; i < mPendingDeallocations[frameIndex].size(); i++) {
			auto* deallocation = mPendingDeallocations[frameIndex][i];
			mBuddyHeapPool.Deallocate(deallocation->poolAllocation);
			delete deallocation;
		}
		mPendingDeallocations[frameIndex].clear();
	}

}