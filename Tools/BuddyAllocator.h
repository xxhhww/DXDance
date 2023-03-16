#pragma once
#include <vector>
#include <set>
#include <memory>
#include <optional>

namespace Tool {

	template<typename BlockUserDataType>
	struct BuddyBlock {
		BuddyBlock(size_t offset, size_t size) : offset(offset), size(size) {}

		size_t offset; // Block的偏移量
		size_t size;   // Block的字节长度

		BlockUserDataType userData;
	};

	/*
	* 特化模版<void>
	*/
	template<>
	struct BuddyBlock<void> {
		BuddyBlock(size_t offset, size_t size) : offset(offset), size(size) {}

		size_t offset;
		size_t size;
	};

	template<typename BlockUserDataType = void>
	class BuddyAllocator {
	public:
		using Block = BuddyBlock<BlockUserDataType>;

	public:
		BuddyAllocator(size_t minBlockSize, size_t maxBlockSize);
		~BuddyAllocator() = default;

		/*
		* 无可分配空间时返回nullptr
		*/
		Block* Allocate(size_t size);

		void Deallocate(Block* block);

	private:
		/*
		* 递归函数，在order层级寻找可用空间，如果没有，则将order+1层的空间进行对半分隔，当不存在可分配空间时返回nullopt，否则返回其在总块中的偏移量
		*/
		std::optional<size_t> AllocateInternal(uint32_t order);
		
		void DeallocateInternal(size_t offset, uint32_t order);

		size_t SizeToUnitSize(size_t size);

		uint32_t UnitSizeToOrder(size_t size);

		size_t OrderToUnitSize(uint32_t order);

		size_t UnitSizeToSize(size_t size);

		size_t GetBuddyOffset(size_t unitOffset, size_t unitSize);
	private:
		size_t mMinBlockSize;
		size_t mMaxBlockSize; // 未分割时的总块长度
		uint32_t mMaxOrder;
		std::vector<std::set<size_t>> mFreeBlocks; // set中存储块的offset
	};

}

#include "BuddyAllocator.inl"