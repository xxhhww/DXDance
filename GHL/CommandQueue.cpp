#include "CommandQueue.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {
	CommandQueue::CommandQueue(const Device* device, D3D12_COMMAND_LIST_TYPE type)
	: mDevice(device) 
	, mType(type) {
		mDesc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
		mDesc.Priority = D3D12_COMMAND_QUEUE_PRIORITY_NORMAL;
		mDesc.NodeMask = device->GetNodeMask();
		mDesc.Type = mType;

		HRASSERT(mDevice->D3DDevice()->CreateCommandQueue(&mDesc, IID_PPV_ARGS(&mQueue)));
	}

	void CommandQueue::SignalFence(const Fence& fence, std::optional<uint64_t> expectedValue) {
		HRASSERT(mQueue->Signal(fence.D3DFence(), expectedValue.value_or(fence.ExpectedValue())));
	}

	void CommandQueue::WaitFence(const Fence& fence, std::optional<uint64_t> expectedValue) {
		HRASSERT(mQueue->Wait(fence.D3DFence(), expectedValue.value_or(fence.ExpectedValue())));
	}

	void CommandQueue::ExecuteCommandList(ID3D12GraphicsCommandList4* const commandList) {
		mQueue->ExecuteCommandLists(1, (ID3D12CommandList* const*)&commandList);
	}

	void CommandQueue::ExecuteCommandLists(ID3D12GraphicsCommandList4* const commandLists, UINT count) {
		mQueue->ExecuteCommandLists(count, (ID3D12CommandList* const*)&commandLists);
	}

	uint64_t CommandQueue::GetTimestampFrequency() const {
		uint64_t frequency{ 0u };
		HRASSERT(mQueue->GetTimestampFrequency(&frequency));
		return frequency;
	}

	void CommandQueue::SetDebugName(const std::string& name) {
		mQueue->SetName(Tool::StrUtil::UTF8ToWString(name).c_str());
	}

	GraphicsQueue::GraphicsQueue(const Device* device)
	: CommandQueue(device, D3D12_COMMAND_LIST_TYPE_DIRECT) {}

	ComputeQueue::ComputeQueue(const Device* device)
	: CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COMPUTE) {}

	CopyQueue::CopyQueue(const Device* device)
	: CommandQueue(device, D3D12_COMMAND_LIST_TYPE_COPY) {}
}