#pragma once
#include <vector>
#include <set>
#include <memory>
#include <optional>

namespace Tool {

	template<typename BlockUserDataType>
	struct BuddyBlock {
		BuddyBlock(size_t offset, size_t size) : offset(offset), size(size) {}

		size_t offset; // Block��ƫ����
		size_t size;   // Block���ֽڳ���

		BlockUserDataType userData;
	};

	/*
	* �ػ�ģ��<void>
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
		* �޿ɷ���ռ�ʱ����nullptr
		*/
		Block* Allocate(size_t size);

		void Deallocate(Block* block);

	private:
		/*
		* �ݹ麯������order�㼶Ѱ�ҿ��ÿռ䣬���û�У���order+1��Ŀռ���ж԰�ָ����������ڿɷ���ռ�ʱ����nullopt�����򷵻������ܿ��е�ƫ����
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
		size_t mMaxBlockSize; // δ�ָ�ʱ���ܿ鳤��
		uint32_t mMaxOrder;
		std::vector<std::set<size_t>> mFreeBlocks; // set�д洢���offset
	};

}

#include "BuddyAllocator.inl"