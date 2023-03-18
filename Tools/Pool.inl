#pragma once
#include "Pool.h"

namespace Tool {

	template<typename SlotUserDataType>
	typename Pool<SlotUserDataType>::Slot* Pool<SlotUserDataType>::Allocate() {
		if (mRetiredIDs.empty()) {
			mSlots.emplace_back(new Slot(mSlots.size()));
			return mSlots.back().get();
		}

		uint64_t id = mRetiredIDs.front();
		mRetiredIDs.pop();
		return *mSlots.at(id).get();
	}

	template<typename SlotUserDataType>
	void Pool<SlotUserDataType>::Deallocate(typename Pool<SlotUserDataType>::Slot* slot) {
		mRetiredIDs.push(slot->id);
	}

}