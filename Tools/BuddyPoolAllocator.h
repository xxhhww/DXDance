#pragma once
#include "BuddyPool.h"

namespace Tool {


	template<typename BucketUserDataType, typename BlockUserDataType>
	struct BuddyPoolBucket {
		uint32_t                     bucketIndex;
		size_t                       bucketSize;
		BuddyPool<BlockUserDataType> pool;

		BucketUserDataType           userData;
	};

	template<typename BucketUserDataType>
	struct BuddyPoolBucket<BucketUserDataType, void> {
		uint32_t                     bucketIndex;
		size_t                       bucketSize;
		BuddyPool<>                  pool;

		BucketUserDataType           userData;
	};

	template<typename BlockUserDataType>
	struct BuddyPoolBucket<void, BlockUserDataType> {
		uint32_t                     bucketIndex;
		size_t                       bucketSize;
		BuddyPool<BlockUserDataType> pool;
	};

	template<>
	struct BuddyPoolBucket<void, void> {
		uint32_t                     bucketIndex;
		size_t                       bucketSize;
		BuddyPool<>                  pool;
	};

	/*
	* 通用的伙伴池模板
	*/
	template<typename BucketUserDataType = void, typename BlockUserDataType = void>
	class BuddyPoolAllocator {
	public:
		using Bucket = BuddyPoolBucket<BucketUserDataType, BlockUserDataType>;

		struct Allocation {
		public:
			Allocation(Bucket& bucket, typename BuddyPool<BlockUserDataType>::Block& block) : bucket(bucket), block(block) {}

			Bucket& bucket;
			typename BuddyPool<BlockUserDataType>::Block& block;
		};

	public:
		BuddyPoolAllocator(size_t minBlockSize, size_t maxBlockSize) : mMinBlockSize(minBlockSize), mMaxBlockSize(maxBlockSize) {}
		~BuddyPoolAllocator() = default;

		Allocation Allocate(size_t size);
		
		void Deallocate(Allocation& allocation);

	private:
		size_t mMinBlockSize;
		size_t mMaxBlockSize;

		std::vector<std::unique_ptr<Bucket>> mBuckets;
	};


}

#include "BuddyPoolAllocator.inl"