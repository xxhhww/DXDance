#include "Fence.h"
#include "Tools/Assert.h"
#include "Tools/StrUtil.h"

namespace GHL {
	Fence::Fence(const Device* device) 
	: mDevice(device) {
		HRASSERT(mDevice->D3DDevice()->CreateFence(0u, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(&mFence)));
	}

	uint64_t Fence::IncrementExpectedValue() {
		return ++mExpectedValue;
	}

	bool Fence::IsCompleted() const {
		return mFence->GetCompletedValue() >= mExpectedValue;
	}

	void Fence::SetCompletionEvent(uint64_t value, HANDLE eventHandle) {
		HRASSERT(mFence->SetEventOnCompletion(value, eventHandle));
	}

	void Fence::SetDebugName(const std::string& name) {
		HRASSERT(mFence->SetName(Tool::StrUtil::UTF8ToWString(name).c_str()));
	}
}