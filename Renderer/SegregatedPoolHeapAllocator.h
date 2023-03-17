#pragma once
#include "GHL/Heap.h"
#include "Tools/SegregatedPool.h"
#include <optional>

namespace GHL {

	/*
	* �ѷ�����
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

		size_t mMinimumHeapSize{ 0u }; // �ѵ���Сֵ

		/*
		* �ϴ��ѷ����
		*/
		SegregatedPool mUploadHeapPools;
		std::vector<HeapPtrArray> mUploadHeapLists;

		/*
		* �ض��ѷ����
		*/
		SegregatedPool mReadBackHeapPools;
		std::vector<HeapPtrArray> mReadBackHeapLists;
	
		/*
		* Ĭ�϶ѷ����
		*/
		SegregatedPool mDefaultHeapPools;
		std::vector<HeapPtrArray> mDefaultHeapLists;
	};

}