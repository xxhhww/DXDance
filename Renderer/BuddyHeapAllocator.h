#pragma once
#include "GHL/Device.h"
#include "GHL/Heap.h"
#include "Tools/BuddyAllocatorPool.h"
#include "RingFrameTracker.h"
#include <optional>

namespace Renderer {

	/*
	* �ػ��Ķѷ�����������ѵĴ��������ã�������ѵ��ͷ�
	*/
	class BuddyHeapAllocator {
	public:
		struct BucketUserData {
			std::optional<uint64_t> heapIndex = std::nullopt;
		};

		using BuddyPool = Tool::BuddyAllocatorPool<BucketUserData, void>;

		struct Allocation {
			Allocation(typename BuddyPool::Allocation* poolAllocation, GHL::Heap* heap, uint64_t heapIndex, size_t offset, size_t size)
			: poolAllocation(poolAllocation)
			, heap(heap)
			, heapOffset(offset)
			, size(size) {}

			~Allocation() = default;

			typename BuddyPool::Allocation* poolAllocation{ nullptr };

			GHL::Heap* heap{ nullptr };
			uint64_t   heapIndex{ 0u };
			size_t     heapOffset{ 0u };
			uint32_t   tileOffset{ 0u };
			size_t     size{ 0u };
		};

	public:
		BuddyHeapAllocator(const GHL::Device* device, RingFrameTracker* ringFrameTracker);
		BuddyHeapAllocator(const BuddyHeapAllocator& other) = delete;
		BuddyHeapAllocator(BuddyHeapAllocator&& other) = default;
		BuddyHeapAllocator& operator=(const BuddyHeapAllocator& other) = delete;
		BuddyHeapAllocator& operator=(BuddyHeapAllocator&& other) = default;
		~BuddyHeapAllocator() = default;

		Allocation* Allocate(size_t memorySize);

		/*
		* �ӳٸ��¶ѵ�ʹ��״̬(����������)
		*/
		void Deallocate(Allocation* allocation);

	private:
		void CleanUpPendingDeallocation(uint8_t frameIndex);

	private:
		const GHL::Device* mDevice{ nullptr };
		RingFrameTracker* mFrameTracker{ nullptr };

		size_t mMinBlockSize; // ��С��
		size_t mMaxBlockSize; // ���飬Ҳ����һ��Heap�Ĵ�С

		BuddyPool mBuddyHeapPool;
		std::vector<std::unique_ptr<GHL::Heap>> mHeaps;

		std::vector<std::vector<Allocation*>> mPendingDeallocations;
	};

}