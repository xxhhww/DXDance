#pragma once
#include "GHL/Heap.h"
#include "Tools/SegregatedPool.h"
#include <optional>

namespace GHL {

	/*
	* 堆分配器
	*/
	class SegregatedPoolHeapAllocator {
	public:
		struct BucketUserData {
			std::optional<uint32_t> heapListIndex = std::nullopt;
		};

		struct SlotUserData {
			std::optional<uint32_t> heapIndex = std::nullopt;
		};

		using SegregatedPool = Tool::SegregatedPool<BucketUserData, SlotUserData>;

		using HeapPtrArray = std::vector<std::unique_ptr<Heap>>;

		struct Allocation {
			SegregatedPool::Allocation poolAllocation;
			Heap* heap;
		};

	public:
		SegregatedPoolHeapAllocator(const Device* device);
		SegregatedPoolHeapAllocator(const SegregatedPoolHeapAllocator& other) = delete;
		SegregatedPoolHeapAllocator(SegregatedPoolHeapAllocator&& other) = default;
		SegregatedPoolHeapAllocator& operator= (const SegregatedPoolHeapAllocator& other) = delete;
		SegregatedPoolHeapAllocator& operator= (SegregatedPoolHeapAllocator&& other) = default;

		~SegregatedPoolHeapAllocator() = default;

		[[nodiscard]] Allocation Allocate(EResourceUsage usage, size_t heapSize);

		void Deallocate(Allocation& allocation);

	private:
		const Device* mDevice{ nullptr };

		size_t mMinimumHeapSize{ 0u }; // 堆的最小值

		/*
		* 上传堆分配池
		*/
		SegregatedPool mUploadHeapPools;
		std::vector<HeapPtrArray> mUploadHeapLists;

		/*
		* 回读堆分配池
		*/
		SegregatedPool mReadBackHeapPools;
		std::vector<HeapPtrArray> mReadBackHeapLists;
	
		/*
		* 默认堆分配池
		*/
		SegregatedPool mDefaultHeapPools;
		std::vector<HeapPtrArray> mDefaultHeapLists;
	};

}