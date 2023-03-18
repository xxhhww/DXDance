#pragma once
#include <vector>
#include <queue>
#include <memory>

namespace Tool {

	template<typename SlotUserDataType>
	struct PoolSlot {
		PoolSlot(uint64_t id) : id(id) {}

		uint64_t id;
		SlotUserDataType userData;
	};

	/*
	* 特化模板 <void>
	*/
	template<>
	struct PoolSlot<void> {
		PoolSlot(uint64_t id) : id(id) {}

		uint64_t id;
	};

	/*
	* 通用的池模板
	*/
	template<typename SlotUserDataType = void>
	class Pool {
	public:
		using Slot = PoolSlot<SlotUserDataType>;

	public:
		Slot* Allocate();

		void Deallocate(Slot* slot);

	private:
		std::vector<std::unique_ptr<Slot>> mSlots;
		std::queue<uint64_t>               mRetiredIDs;
	};

}

#include "Pool.inl"