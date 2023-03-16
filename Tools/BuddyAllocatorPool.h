#pragma once
#include "BuddyAllocator.h"

namespace Tool {

	template<typename BucketUserDataType, typename BlockUserDataType>
	struct BuddyPoolBucket {
		BuddyPoolBucket(uint64_t index, size_t minBlockSize, size_t maxBlockSize) : bucketIndex(index), bucketSize(maxBlockSize), pool(minBlockSize, maxBlockSize) {}

		uint32_t                          bucketIndex;
		size_t                            bucketSize;
		BuddyAllocator<BlockUserDataType> pool;

		BucketUserDataType                userData;
	};

	template<typename BucketUserDataType>
	struct BuddyPoolBucket<BucketUserDataType, void> {
		BuddyPoolBucket(uint64_t index, size_t minBlockSize, size_t maxBlockSize) : bucketIndex(index), bucketSize(maxBlockSize), pool(minBlockSize, maxBlockSize) {}

		uint32_t                     bucketIndex;
		size_t                       bucketSize;
		BuddyAllocator<>             pool;

		BucketUserDataType           userData;
	};

	template<typename BlockUserDataType>
	struct BuddyPoolBucket<void, BlockUserDataType> {
		BuddyPoolBucket(uint64_t index, size_t minBlockSize, size_t maxBlockSize) : bucketIndex(index), bucketSize(maxBlockSize), pool(minBlockSize, maxBlockSize) {}

		uint32_t                          bucketIndex;
		size_t                            bucketSize;
		BuddyAllocator<BlockUserDataType> pool;
	};

	template<>
	struct BuddyPoolBucket<void, void> {
		BuddyPoolBucket(uint64_t index, size_t minBlockSize, size_t maxBlockSize) : bucketIndex(index), bucketSize(maxBlockSize), pool(minBlockSize, maxBlockSize) {}

		uint32_t                     bucketIndex;
		size_t                       bucketSize;
		BuddyAllocator<>             pool;
	};

	/*
	* ͨ�õĻ���ģ��
	*/
	template<typename BucketUserDataType = void, typename BlockUserDataType = void>
	class BuddyAllocatorPool {
	public:
		using Bucket = BuddyPoolBucket<BucketUserDataType, BlockUserDataType>;

		struct Allocation {
		public:
			Allocation(Bucket* bucket, typename BuddyAllocator<BlockUserDataType>::Block* block) : bucket(bucket), block(block) {}

			Bucket* bucket;
			typename BuddyAllocator<BlockUserDataType>::Block* block;
		};

	public:
		BuddyAllocatorPool(size_t minBlockSize, size_t maxBlockSize) : mMinBlockSize(minBlockSize), mMaxBlockSize(maxBlockSize) {}
		~BuddyAllocatorPool() = default;

		Allocation* Allocate(size_t size) {
			ASSERT_FORMAT(size <= mMaxBlockSize, "Too Large Size!");

			Allocation* allocation = nullptr;

			for (auto& bucket : mBuckets) {
				// �������е�bucket��Ѱ�ҿɷ���ռ�
				typename BuddyAllocator<BlockUserDataType>::Block* block = nullptr;
				block = bucket->pool.Allocate(size);
				if (block != nullptr) {
					return new Allocation(bucket.get(), block);
				}
			}

			// �޿ɷ���ռ䣬����һ���µ�bucket
			mBuckets.emplace_back(new Bucket(mBuckets.size(), mMinBlockSize, mMaxBlockSize));
			typename BuddyAllocator<BlockUserDataType>::Block* block = mBuckets.back()->pool.Allocate(size);

			return new Allocation(mBuckets.back().get(), block);
		}
		
		void Deallocate(Allocation* allocation) {
			allocation->bucket->pool.Deallocate(allocation->block);

			delete allocation;
		}

	private:
		size_t mMinBlockSize;
		size_t mMaxBlockSize; // ÿһ��Bucket�ı�׼��С
		std::vector<std::unique_ptr<Bucket>> mBuckets;
	};


}

#include "BuddyAllocatorPool.inl"