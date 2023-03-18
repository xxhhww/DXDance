#pragma once
#include "Pool.h"

namespace Tool {

	template<typename BucketUserDataType = void, typename SlotUserDataType = void>
	struct SegregatedPoolBucket {
	public:
		SegregatedPoolBucket(uint64_t index, size_t size) : bucketIndex(index), bucketSize(size) {}

		uint64_t               bucketIndex;
		size_t                 bucketSize;
		Pool<SlotUserDataType> pool;

		BucketUserDataType     userData;
	};

	/*
	* 特化模板 <void, T>
	*/
	template<typename SlotUserDataType>
	struct SegregatedPoolBucket<void, SlotUserDataType> {
	public:
		SegregatedPoolBucket(uint64_t index, size_t size) : bucketIndex(index), bucketSize(size) {}

		uint64_t               bucketIndex;
		size_t                 bucketSize;
		Pool<SlotUserDataType> pool;
	};

	/*
	* 特化模板 <T, void>
	*/
	template<typename BucketUserDataType>
	struct SegregatedPoolBucket<BucketUserDataType, void> {
	public:
		SegregatedPoolBucket(uint64_t index, size_t size) : bucketIndex(index), bucketSize(size) {}

		uint64_t               bucketIndex;
		size_t                 bucketSize;
		Pool<>                 pool;

		BucketUserDataType     userData;
	};

	/*
	* 特化模板 <void, void>
	*/
	template<>
	struct SegregatedPoolBucket<void, void> {
	public:
		SegregatedPoolBucket(uint64_t index, size_t size) : bucketIndex(index), bucketSize(size) {}

		uint64_t               bucketIndex;
		size_t                 bucketSize;
		Pool<>                 pool;
	};

	/*
	* 通用的分隔池模板
	*/
	template<typename BucketUserDataType = void, typename SlotUserDataType = void>
	class SegregatedPool {
	public:
		using Bucket = SegregatedPoolBucket<BucketUserDataType, SlotUserDataType>;

		struct Allocation {
		public:
			Allocation(Bucket* bucket, typename Pool<SlotUserDataType>::Slot* slot) : bucket(bucket), slot(slot) {}

			Bucket* bucket;
			typename Pool<SlotUserDataType>::Slot* slot;
		};

	public:
		/*
		* 分隔池构造函数，基数默认为2.0f
		*/
		SegregatedPool(size_t cardinality = 2.0f) : mCardinality(cardinality) {}
		~SegregatedPool() = default;

		Allocation Allocate(size_t size);

		void Deallocate(Allocation& allocation);

	private:
		/*
		* 计算Bucket索引
		*/
		uint32_t CalculateBucketIndex(uint64_t size);

	private:
		size_t mCardinality; // 分隔池的基数
		std::vector<std::unique_ptr<Bucket>> mBuckets;
	};

}

#include "SegregatedPool.inl"