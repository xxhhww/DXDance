#include "Fence.h"
#include "Tools/Assert.h"

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
}