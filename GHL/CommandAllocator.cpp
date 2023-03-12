#include "CommandAllocator.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {
	CommandAllocator::CommandAllocator(const Device* device, D3D12_COMMAND_LIST_TYPE type)
	: mDevice(device)
	, mType(type) {
		HRASSERT(mDevice->D3DDevice()->CreateCommandAllocator(mType, IID_PPV_ARGS(&mAllocator)));
	}

	void CommandAllocator::Reset() {
		HRASSERT(mAllocator->Reset());
	}

	void CommandAllocator::SetDebugName(const std::string& name) {
		HRASSERT(mAllocator->SetName(Tool::StrUtil::UTF8ToWString(name).c_str()));
	}
}