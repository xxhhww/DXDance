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
	* �ػ�ģ�� <void, T>
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
	* �ػ�ģ�� <T, void>
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
	* �ػ�ģ�� <void, void>
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
	* ͨ�õķָ���ģ��
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
		* �ָ��ع��캯��������Ĭ��Ϊ2.0f
		*/
		SegregatedPool(size_t cardinality = 2.0f) : mCardinality(cardinality) {}
		~SegregatedPool() = default;

		Allocation Allocate(size_t size);

		void Deallocate(Allocation& allocation);

	private:
		/*
		* ����Bucket����
		*/
		uint32_t CalculateBucketIndex(uint64_t size);

	private:
		size_t mCardinality; // �ָ��صĻ���
		std::vector<std::unique_ptr<Bucket>> mBuckets;
	};

}

#include "SegregatedPool.inl"