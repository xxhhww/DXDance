#include "PoolCommandListAllocator.h"
#include "Tools/Assert.h"

namespace Renderer {

	PoolCommandListAllocator::PoolCommandListAllocator(const Device* device)
	: mDevice(device) {}

	PoolCommandListAllocator::Allocation PoolCommandListAllocator::GetGraphicsCommandList() {
		auto& slot = mGraphicsPool.Allocate();
		auto& slotUserData = slot.userData;

		if (!slotUserData.index) {
			// 新的索引
			mGraphicsCommandAllocators.emplace_back(new CommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT));
			mGraphicsCommandLists.emplace_back(new CommandList(mDevice, mGraphicsCommandAllocators.back()->D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_DIRECT));
			slotUserData.index = mGraphicsCommandLists.size() - 1;
		}
		else {
			// 旧的索引，使用之前需要重置命令分配器与命令列表
			mGraphicsCommandAllocators.at(*slotUserData.index)->Reset();
			mGraphicsCommandLists.at(*slotUserData.index)->Reset();
		}

		return Allocation{
			slot,
			mGraphicsCommandLists.at(*slotUserData.index).get()
		};
	}

	PoolCommandListAllocator::Allocation PoolCommandListAllocator::GetComputeCommandList() {
		auto& slot = mComputePool.Allocate();
		auto& slotUserData = slot.userData;

		if (!slotUserData.index) {
			// 新的索引
			mComputeCommandAllocators.emplace_back(new CommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE));
			mComputeCommandLists.emplace_back(new CommandList(mDevice, mComputeCommandAllocators.back()->D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_COMPUTE));
			slotUserData.index = mComputeCommandLists.size() - 1;
		}
		else {
			// 旧的索引，使用之前需要重置命令分配器与命令列表
			mComputeCommandAllocators.at(*slotUserData.index)->Reset();
			mComputeCommandLists.at(*slotUserData.index)->Reset();
		}

		return Allocation{
			slot,
			mComputeCommandLists.at(*slotUserData.index).get()
		};
	}

	PoolCommandListAllocator::Allocation PoolCommandListAllocator::GetCopyCommandList() {
		auto& slot = mCopyPool.Allocate();
		auto& slotUserData = slot.userData;

		if (!slotUserData.index) {
			// 新的索引
			mCopyCommandAllocators.emplace_back(new CommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_COPY));
			mCopyCommandLists.emplace_back(new CommandList(mDevice, mCopyCommandAllocators.back()->D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_COPY));
			slotUserData.index = mCopyCommandLists.size() - 1;
		}
		else {
			// 旧的索引，使用之前需要重置命令分配器与命令列表
			mCopyCommandAllocators.at(*slotUserData.index)->Reset();
			mCopyCommandLists.at(*slotUserData.index)->Reset();
		}

		return Allocation{
			slot,
			mCopyCommandLists.at(*slotUserData.index).get()
		};
	}

	void PoolCommandListAllocator::RetireCommandList(PoolCommandListAllocator::Allocation& allocation) {
		switch (allocation.commandList->GetListType()) {
		case D3D12_COMMAND_LIST_TYPE_DIRECT:
			mGraphicsPool.Deallocate(allocation.poolAllocation);
			break;
		case D3D12_COMMAND_LIST_TYPE_COMPUTE:
			mComputePool.Deallocate(allocation.poolAllocation);
			break;
		case D3D12_COMMAND_LIST_TYPE_COPY:
			mCopyPool.Deallocate(allocation.poolAllocation);
			break;
		default:
			ASSERT_FORMAT(false, "Unsupported Command List Type");
			break;
		}
	}
}