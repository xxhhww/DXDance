#include "CommandList.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {

	CommandList::CommandList(const Device* device, ID3D12CommandAllocator* allocator, D3D12_COMMAND_LIST_TYPE type)
	: mDevice(device) 
	, mAllocator(allocator)
	, mType(type) {
		HRASSERT(mDevice->D3DDevice()->CreateCommandList(mDevice->GetNodeMask(), mType, mAllocator, nullptr, IID_PPV_ARGS(&mCommandList)));
	}

	void CommandList::Reset() {
		HRASSERT(mCommandList->Reset(mAllocator, nullptr));
	}

	void CommandList::Close() {
		HRASSERT(mCommandList->Close());
	}

	void CommandList::SetDebugName(const std::string& name) {
		HRASSERT(mCommandList->SetName(Tool::StrUtil::UTF8ToWString(name).c_str()));
	}

}