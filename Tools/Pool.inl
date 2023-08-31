#pragma once
#include "Pool.h"

namespace Tool {

	template<typename SlotUserDataType>
	typename Pool<SlotUserDataType>::Slot* Pool<SlotUserDataType>::Allocate() {
		typename Pool<SlotUserDataType>::Slot* slot = nullptr;
		{
			std::lock_guard lock(mMutex);

			if (mRetiredIDs.empty()) {
				mSlots.emplace_back(new Slot(mSlots.size()));
				slot = mSlots.back().get();
			}
			else {
				uint64_t id = mRetiredIDs.front();
				mRetiredIDs.pop();
				slot = mSlots.at(id).get();
			}
		}

		return slot;
	}

	template<typename SlotUserDataType>
	void Pool<SlotUserDataType>::Deallocate(typename Pool<SlotUserDataType>::Slot* slot) {
		{
			std::lock_guard lock(mMutex);

			mRetiredIDs.push(slot->id);
		}
	}

	template<typename SlotUserDataType>
	size_t Pool<SlotUserDataType>::AllocatedSize() {
		size_t size = 0u;
		{
			std::lock_guard lock(mMutex);

			size = mSlots.size() - mRetiredIDs.size();
		}

		return size;
	}

}