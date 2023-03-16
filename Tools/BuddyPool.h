#pragma once
#include <vector>
#include <set>
#include <memory>

namespace Tool {

	template<typename BlockUserDataType>
	struct BuddyBlock {
		size_t offset; // Block��ƫ����
		size_t size;   // Block���ֽڳ���

		BlockUserDataType userData;
	};

	/*
	* �ػ�ģ��<void>
	*/
	template<>
	struct BuddyBlock<void> {
		size_t blockOffset;
		size_t blockSize;
	};

	template<typename BlockUserDataType = void>
	class BuddyAllocator {
	public:
		using Block = BuddyBlock<BlockUserDataType>;

	public:
		BuddyAllocator(size_t minBlockSize, size_t maxBlockSize);

		Block& Allocate(size_t size);

		void Deallocate(Block& block);

	private:
		size_t mMinBlockSize;
		size_t mMaxBlockSize; // δ�ָ�ʱ���ܿ鳤��
		uint32_t mMaxOrder;
		std::vector<std::set<std::unique_ptr<Block>>> mFreeBlocks;
	};

}

#include "BuddyAllocator.inl"