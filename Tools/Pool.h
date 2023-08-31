#pragma once
#include <vector>
#include <queue>
#include <memory>
#include <mutex>

namespace Tool {

	template<typename SlotUserDataType>
	struct PoolSlot {
		PoolSlot(uint64_t id) : id(id) {}

		uint64_t id;
		SlotUserDataType userData;
	};

	/*
	* �ػ�ģ�� <void>
	*/
	template<>
	struct PoolSlot<void> {
		PoolSlot(uint64_t id) : id(id) {}

		uint64_t id;
	};

	/*
	* ͨ�õĳ�ģ��
	*/
	template<typename SlotUserDataType = void>
	class Pool {
	public:
		using Slot = PoolSlot<SlotUserDataType>;

	public:
		Slot* Allocate();

		void Deallocate(Slot* slot);

		/*
		* ��ȡ������������ʹ�õ�Slot�ĸ���
		*/
		size_t AllocatedSize();

	private:
		std::mutex mMutex;
		std::vector<std::unique_ptr<Slot>> mSlots;
		std::queue<uint64_t>               mRetiredIDs;
	};

}

#include "Pool.inl"