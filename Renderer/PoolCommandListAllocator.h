#pragma once
#include "GHL/CommandList.h"
#include "RingFrameTracker.h"
#include "Tools/Pool.h"
#include "Tools/Wrap.h"
#include <optional>

namespace Renderer {

	using CommandListWrap = Tool::Wrap<GHL::CommandList>;

	/*
	* 池化的命令列表分配器，负责命令列表的创建与重用，不负责命令列表的释放
	*/
	class PoolCommandListAllocator {
	public:
		struct SlotUserData {
			std::optional<uint64_t> commandListIndex = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserData>;

		struct Deallocation {
			typename Pool::Slot* slot;
			D3D12_COMMAND_LIST_TYPE listType;
		};

	public:
		PoolCommandListAllocator(const GHL::Device* device, RingFrameTracker* ringFrameTracker);
		PoolCommandListAllocator(const PoolCommandListAllocator& other) = delete;
		PoolCommandListAllocator(PoolCommandListAllocator&& other) = default;
		PoolCommandListAllocator& operator=(const PoolCommandListAllocator& other) = delete;
		PoolCommandListAllocator& operator=(PoolCommandListAllocator&& other) = default;
		~PoolCommandListAllocator() = default;

		CommandListWrap AllocateGraphicsCommandList();
		CommandListWrap AllocateComputeCommandList();
		CommandListWrap AllocateCopyCommandList();

	private:
		void CleanUpPendingDeallocation(uint8_t frameIndex);

	private:
		const GHL::Device* mDevice{ nullptr };
		RingFrameTracker* mFrameTracker{ nullptr };

		Pool mGraphicsPool;
		std::vector<std::unique_ptr<GHL::CommandAllocator>> mGraphicsCommandAllocators;
		std::vector<std::unique_ptr<GHL::CommandList>> mGraphicsCommandLists;

		Pool mComputePool;
		std::vector<std::unique_ptr<GHL::CommandAllocator>> mComputeCommandAllocators;
		std::vector<std::unique_ptr<GHL::CommandList>> mComputeCommandLists;

		Pool mCopyPool;
		std::vector<std::unique_ptr<GHL::CommandAllocator>> mCopyCommandAllocators;
		std::vector<std::unique_ptr<GHL::CommandList>> mCopyCommandLists;

		std::vector<std::vector<Deallocation>> mPendingDeallocations;

	};

}