#pragma once
#include "SegregatedPool.h"
#include <cmath>

namespace Tool {

	template<typename BucketUserDataType, typename SlotUserDataType>
	typename SegregatedPool<BucketUserDataType, SlotUserDataType>::Allocation SegregatedPool<BucketUserDataType, SlotUserDataType>::Allocate(size_t size) {
		uint32_t bucketIndex = CalculateBucketIndex(size);

		if (bucketIndex >= mBuckets.size()) {
			uint32_t numberOfBucketsToAdd = (bucketIndex + 1) - mBuckets.size();

			for (uint32_t i = 0; i < numberOfBucketsToAdd; i++) {

				uint64_t newBucketIndex = mBuckets.size();
				size_t newBucketSize = std::powf(mCardinality, newBucketIndex + 1);
				mBuckets.emplace_back(std::make_unique<Bucket>(newBucketIndex, newBucketSize));
			}
		}

		return Allocation{
			mBuckets.at(bucketIndex).get(),
			mBuckets.at(bucketIndex)->pool.Allocate()
		};
	}

	template<typename BucketUserDataType, typename SlotUserDataType>
	void SegregatedPool<BucketUserDataType, SlotUserDataType>::Deallocate(SegregatedPool<BucketUserDataType, SlotUserDataType>::Allocation& allocation) {
		// mBuckets.at(allocation.bucket.pool.Deallocate(allocation.slot));
	}


	template<typename BucketUserDataType, typename SlotUserDataType>
	uint32_t SegregatedPool<BucketUserDataType, SlotUserDataType>::CalculateBucketIndex(uint64_t size) {
		if (size < mCardinality) {
			return 0;
		}

		auto logValue = std::log2f((float)size / mCardinality);
		return std::ceil(logValue);
	}


}