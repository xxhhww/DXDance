#include "SegregatedPoolHeapAllocator.h"
#include "Tools/Assert.h"

namespace GHL {
	SegregatedPoolHeapAllocator::SegregatedPoolHeapAllocator(const Device* device) 
	: mDevice(device) 
	, mMinimumHeapSize(mDevice->GetMinimumHeapSize())
	, mUploadHeapPools(mMinimumHeapSize) 
	, mReadBackHeapPools(mMinimumHeapSize)
	, mDefaultHeapPools(mMinimumHeapSize) {}

	SegregatedPoolHeapAllocator::Allocation SegregatedPoolHeapAllocator::Allocate(EResourceUsage usage, size_t heapSize) {
		SegregatedPool* pool = nullptr;
		std::vector<HeapPtrArray>* array = nullptr;

		switch (usage) {
		case GHL::EResourceUsage::Upload:
			pool = &mUploadHeapPools;
			array = &mUploadHeapLists;
			break;
		case GHL::EResourceUsage::ReadBack:
			pool = &mReadBackHeapPools;
			array = &mReadBackHeapLists;
			break;
		case GHL::EResourceUsage::Default:
			pool = &mDefaultHeapPools;
			array = &mDefaultHeapLists;
			break;
		default:
			ASSERT_FORMAT(false, "Unsupported Heap Type");
			break;
		}

		auto poolAllocation = pool->Allocate(heapSize);
		auto& bucket = poolAllocation.bucket;
		auto& slot = poolAllocation.slot;
		auto& bucketUserData = bucket->userData;
		auto& slotUserData = slot->userData;

		if (!bucketUserData.heapListIndex) {
			array->emplace_back();
			bucketUserData.heapListIndex = array->size() - 1;
		}

		auto& heapPtrArray = array->at(*bucketUserData.heapListIndex);

		if (!slotUserData.heapIndex) {
			heapPtrArray.emplace_back(new Heap(mDevice, bucket->bucketSize, usage));
			slotUserData.heapIndex = heapPtrArray.size() - 1;
		}

		return Allocation{
			poolAllocation,
			heapPtrArray.at(*slotUserData.heapIndex).get()
		};
	}

	void SegregatedPoolHeapAllocator::Deallocate(Allocation& allocation) {
		SegregatedPool* pool = nullptr;

		switch (allocation.heap->GetUsage()) {
		case GHL::EResourceUsage::Upload:
			pool = &mUploadHeapPools;
			break;
		case GHL::EResourceUsage::ReadBack:
			pool = &mReadBackHeapPools;
			break;
		case GHL::EResourceUsage::Default:
			pool = &mDefaultHeapPools;
			break;
		default:
			ASSERT_FORMAT(false, "Unsupported Heap Type");
			break;
		}

		pool->Deallocate(allocation.poolAllocation);
	}
}