#include "CommandListAllocator.h"

namespace GHL {

	CommandListAllocator::CommandListAllocator(const Device* device)
	: mDevice(device) {}

	ID3D12GraphicsCommandList4* CommandListAllocator::GetGraphicsCommandList() {
		auto& slot = mGraphicsPool.Allocate();
		auto& slotUserData = slot.userData;

		if (!slotUserData.index) {
			// ÐÂµÄË÷Òý
			mGraphicsCommandAllocators.emplace_back(mDevice, D3D12_COMMAND_LIST_TYPE_DIRECT);
			mGraphicsCommandLists.emplace_back(mDevice, mGraphicsCommandAllocators.back().D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_DIRECT);
			slotUserData.index = mGraphicsCommandLists.size() - 1;
		}

		return mGraphicsCommandLists.at(*slotUserData.index).D3DCommandList();
	}

	ID3D12GraphicsCommandList4* CommandListAllocator::GetComputeCommandList() {

	}

	ID3D12GraphicsCommandList4* CommandListAllocator::GetCopyCommandList() {

	}

}