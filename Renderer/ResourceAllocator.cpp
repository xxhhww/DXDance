#include "ResourceAllocator.h"

namespace Renderer {

	ResourceAllocator::ResourceAllocator(
		const GHL::Device* device,
		RingFrameTracker* frameTracker,
		BuddyHeapAllocator* heapAllocator,
		PoolDescriptorAllocator* descriptorAllocator)
	: mDevice(device)
	, mFrameTracker(frameTracker)
	, mHeapAllocator(heapAllocator)
	, mDescriptorAllocator(descriptorAllocator) {

		mFrameTracker->AddFrameCompletedCallBack([this](const size_t& frameIndex) {
			CleanUpPendingDeallocation(frameIndex);
		});

		mPendingDeallocations.resize(mFrameTracker->GetMaxSize());

	}

	ResourceAllocator::~ResourceAllocator() {

	}

	void ResourceAllocator::CleanUpPendingDeallocation(uint8_t frameIndex) {

	}

}