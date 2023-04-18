#pragma once
#include "BuddyAllocator.h"
#include "Assert.h"
#include "Math/Helper.h"

namespace Tool {

	template<typename BlockUserDataType>
	BuddyAllocator<BlockUserDataType>::BuddyAllocator(size_t minBlockSize, size_t maxBlockSize)
	: mMinBlockSize(minBlockSize)
	, mMaxBlockSize(maxBlockSize) {
		ASSERT_FORMAT(Math::IsDivisible(maxBlockSize, minBlockSize), "Must Divisible");
		ASSERT_FORMAT(Math::IsPowerOfTwo(maxBlockSize / minBlockSize), "Must Power Of Two");

		mMaxOrder = UnitSizeToOrder(SizeToUnitSize(mMaxBlockSize));

		mFreeBlocks.clear();
		mFreeBlocks.resize(mMaxOrder + 1u);
		mFreeBlocks.at(mMaxOrder).insert((size_t)0);
	}

	template<typename BlockUserDataType>
	typename BuddyAllocator<BlockUserDataType>::Block* BuddyAllocator<BlockUserDataType>::Allocate(size_t size) {
		ASSERT_FORMAT(size <= mMaxBlockSize, "Too Large Size!");

		size_t unitSize = SizeToUnitSize(size);
		uint32_t order = UnitSizeToOrder(unitSize);

		auto offset = AllocateInternal(order);
		if (!offset) {
			// 无满足要求的可分配空间
			return nullptr;
		}

		return new Block(*offset, UnitSizeToSize(unitSize));
	}

	template<typename BlockUserDataType>
	void BuddyAllocator<BlockUserDataType>::Deallocate(typename BuddyAllocator<BlockUserDataType>::Block* block) {
		size_t offset = block->offset;
		uint32_t order = UnitSizeToOrder(SizeToUnitSize(block->size));

		DeallocateInternal(offset, order);

		delete block;
	}

	template<typename BlockUserDataType>
	std::optional<size_t> BuddyAllocator<BlockUserDataType>::AllocateInternal(uint32_t order) {
		if (order > mMaxOrder) {
			return std::nullopt;
		}

		auto it = mFreeBlocks.at(order).begin();
		if (it == mFreeBlocks.at(order).end()) {
			auto left = AllocateInternal(order + 1);
			if (!left) {
				return std::nullopt;
			}

			size_t right = *left + UnitSizeToSize(OrderToUnitSize(order));
			mFreeBlocks.at(order).insert(right);

			return left;
		}
		else {
			size_t offset = *it;
			mFreeBlocks.at(order).erase(it);
			return offset;
		}

		return std::nullopt;
	}

	template<typename BlockUserDataType>
	void BuddyAllocator<BlockUserDataType>::DeallocateInternal(size_t offset, uint32_t order) {
		size_t unitOffset = SizeToUnitSize(offset);
		size_t unitSize = OrderToUnitSize(order);
		size_t buddy = GetBuddyOffset(unitOffset, unitSize);

		auto it = std::find_if(mFreeBlocks.at(order).begin(), mFreeBlocks.at(order).end(),
			[&](const size_t& size) {
				if (SizeToUnitSize(size) == buddy) {
					return true;
				}
				return false;
			});

		if (it != mFreeBlocks.at(order).end()) {
			// 合并兄弟块
			size_t min = (offset < UnitSizeToSize(buddy)) ? offset : UnitSizeToSize(buddy);
			DeallocateInternal(min, order + 1);
			mFreeBlocks.at(order).erase(it);
		}
		else {
			mFreeBlocks.at(order).insert(offset);
		}
	}

	template<typename BlockUserDataType>
	size_t BuddyAllocator<BlockUserDataType>::SizeToUnitSize(size_t size) {
		return (size + (mMinBlockSize - 1)) / mMinBlockSize;
	}

	template<typename BlockUserDataType>
	uint32_t BuddyAllocator<BlockUserDataType>::UnitSizeToOrder(size_t size) {
		return Math::Log2(size);
	}

	template<typename BlockUserDataType>
	size_t BuddyAllocator<BlockUserDataType>::OrderToUnitSize(uint32_t order) {
		return (size_t)1 << order;
	}

	template<typename BlockUserDataType>
	size_t BuddyAllocator<BlockUserDataType>::UnitSizeToSize(size_t size) {
		return size * mMinBlockSize;
	}

	template<typename BlockUserDataType>
	size_t BuddyAllocator<BlockUserDataType>::GetBuddyOffset(size_t unitOffset, size_t unitSize) {
		return unitOffset ^ unitSize;
	}

}