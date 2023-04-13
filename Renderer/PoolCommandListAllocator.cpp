#include "PoolCommandListAllocator.h"
#include "Tools/Assert.h"

namespace Renderer {

	PoolCommandListAllocator::PoolCommandListAllocator(const GHL::Device* device, RingFrameTracker* ringFrameTracker)
	: mDevice(device)
	, mFrameTracker(ringFrameTracker) {
		mFrameTracker->AddFrameCompletedCallBack([this](const size_t& frameIndex) {
			CleanUpPendingDeallocation(frameIndex);
		});

		mPendingDeallocations.resize(mFrameTracker->GetMaxSize());
	}

	CommandListWrap PoolCommandListAllocator::AllocateCommandList(GHL::EGPUQueue queueType) {
		switch (queueType) {
		case GHL::EGPUQueue::Graphics:	return AllocateGraphicsCommandList();
		case GHL::EGPUQueue::Compute:	return AllocateComputeCommandList();
		case GHL::EGPUQueue::Copy:		return AllocateCopyCommandList();
		default:
			ASSERT_FORMAT(false, "Unsupport COMMAND LIST TYPE");
			break;
		}
	}

	CommandListWrap PoolCommandListAllocator::AllocateGraphicsCommandList() {
		auto* slot = mGraphicsPool.Allocate();
		if (!slot->userData.commandListIndex) {
			mGraphicsCommandAllocators.emplace_back(new GHL::CommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT));
			mGraphicsCommandLists.emplace_back(new GHL::CommandList(mDevice, mGraphicsCommandAllocators.back()->D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_DIRECT));

			slot->userData.commandListIndex = mGraphicsCommandLists.size() - 1;

			Deallocation deallocation{ slot, D3D12_COMMAND_LIST_TYPE_DIRECT };
			return CommandListWrap{
				mGraphicsCommandLists.back().get(),
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}

		auto* commandList = mGraphicsCommandLists.at(*slot->userData.commandListIndex).get();
		auto* commandListAllocator = mGraphicsCommandAllocators.at(*slot->userData.commandListIndex).get();
		commandListAllocator->Reset();
		commandList->Reset();

		Deallocation deallocation{ slot, D3D12_COMMAND_LIST_TYPE_DIRECT };
		return CommandListWrap{
			commandList,
			[this, deallocation]() {
				mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
			}
		};
	}

	CommandListWrap PoolCommandListAllocator::AllocateComputeCommandList() {
		auto* slot = mComputePool.Allocate();
		if (!slot->userData.commandListIndex) {
			mComputeCommandAllocators.emplace_back(new GHL::CommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_COMPUTE));
			mComputeCommandLists.emplace_back(new GHL::CommandList(mDevice, mComputeCommandAllocators.back()->D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_COMPUTE));

			slot->userData.commandListIndex = mComputeCommandLists.size() - 1;

			Deallocation deallocation{ slot, D3D12_COMMAND_LIST_TYPE_COMPUTE };
			return CommandListWrap{
				mComputeCommandLists.back().get(),
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}

		auto* commandList = mComputeCommandLists.at(*slot->userData.commandListIndex).get();
		auto* commandListAllocator = mComputeCommandAllocators.at(*slot->userData.commandListIndex).get();
		commandListAllocator->Reset();
		commandList->Reset();

		Deallocation deallocation{ slot, D3D12_COMMAND_LIST_TYPE_COMPUTE };
		return CommandListWrap{
			commandList,
			[this, deallocation]() {
				mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
			}
		};
	}

	CommandListWrap PoolCommandListAllocator::AllocateCopyCommandList() {
		auto* slot = mCopyPool.Allocate();
		if (!slot->userData.commandListIndex) {
			mCopyCommandAllocators.emplace_back(new GHL::CommandAllocator(mDevice, D3D12_COMMAND_LIST_TYPE_COPY));
			mCopyCommandLists.emplace_back(new GHL::CommandList(mDevice, mCopyCommandAllocators.back()->D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_COPY));

			slot->userData.commandListIndex = mCopyCommandLists.size() - 1;

			Deallocation deallocation{ slot, D3D12_COMMAND_LIST_TYPE_COPY };
			return CommandListWrap{
				mCopyCommandLists.back().get(),
				[this, deallocation]() {
					mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
				}
			};
		}

		auto* commandList = mCopyCommandLists.at(*slot->userData.commandListIndex).get();
		auto* commandListAllocator = mCopyCommandAllocators.at(*slot->userData.commandListIndex).get();
		commandListAllocator->Reset();
		commandList->Reset();

		Deallocation deallocation{ slot, D3D12_COMMAND_LIST_TYPE_COPY };
		return CommandListWrap{
			commandList,
			[this, deallocation]() {
				mPendingDeallocations[mFrameTracker->GetCurrFrameIndex()].push_back(deallocation);
			}
		};
	}

	void PoolCommandListAllocator::CleanUpPendingDeallocation(uint8_t frameIndex) {
		for (const auto& deallocation : mPendingDeallocations[frameIndex]) {
			switch (deallocation.listType) {
			case D3D12_COMMAND_LIST_TYPE_DIRECT:
				mGraphicsPool.Deallocate(deallocation.slot);
				break;
			case D3D12_COMMAND_LIST_TYPE_COMPUTE:
				mComputePool.Deallocate(deallocation.slot);
				break;
			case D3D12_COMMAND_LIST_TYPE_COPY:
				mCopyPool.Deallocate(deallocation.slot);
				break;
			default:
				ASSERT_FORMAT(false, "Unsupport COMMAND LIST TYPE");
				break;
			}
		}
		mPendingDeallocations[frameIndex].clear();
	}

}