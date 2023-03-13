#pragma once
#include "CommandList.h"
#include "Tools/Pool.h"
#include <optional>

namespace GHL {

	class PoolCommandListAllocator {
	public:
		struct SlotUserData {
			std::optional<uint32_t> index = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserData>;

		struct Allocation {
			Pool::Slot poolAllocation;
			CommandList* commandList{ nullptr };
		};

	public:
		PoolCommandListAllocator(const Device* device);
		PoolCommandListAllocator(const PoolCommandListAllocator& other) = delete;
		PoolCommandListAllocator(PoolCommandListAllocator&& other) = default;
		PoolCommandListAllocator& operator=(const PoolCommandListAllocator& other) = delete;
		PoolCommandListAllocator& operator=(PoolCommandListAllocator&& other) = default;
		~PoolCommandListAllocator() = default;

		[[nodiscard]] Allocation GetGraphicsCommandList();

		[[nodiscard]] Allocation GetComputeCommandList();

		[[nodiscard]] Allocation GetCopyCommandList();

		void RetireCommandList(Allocation& allocation);

	private:
		const Device* mDevice{ nullptr };

		/*
		* GraphicsCommandList
		*/
		Pool mGraphicsPool;
		std::vector<std::unique_ptr<CommandList>> mGraphicsCommandLists;
		std::vector<std::unique_ptr<CommandAllocator>> mGraphicsCommandAllocators;

		/*
		* ComputeCommandList
		*/
		Pool mComputePool;
		std::vector<std::unique_ptr<CommandList>> mComputeCommandLists;
		std::vector<std::unique_ptr<CommandAllocator>> mComputeCommandAllocators;

		/*
		* CopyCommandList
		*/
		Pool mCopyPool;
		std::vector<std::unique_ptr<CommandList>> mCopyCommandLists;
		std::vector<std::unique_ptr<CommandAllocator>> mCopyCommandAllocators;
	};

}