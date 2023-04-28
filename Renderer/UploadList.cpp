#include "UploadList.h"

namespace Renderer {

	UploadList::UploadList()
	: mUploadState(State::Free) {}

	void UploadList::SetUploadListState(UploadList::State state) {
		mUploadState = state;
	}

	void UploadList::SetPendingStreamTexture(StreamTexture* pendingTexture) {
		mPendingStreamTexture = pendingTexture;
	}

	void UploadList::PushPendingLoadings(
		const D3D12_TILED_RESOURCE_COORDINATE& coord,
		BuddyHeapAllocator::Allocation* heapAllocation) {
		mPendingLoadings.push_back(coord);
		mHeapAllocations.push_back(heapAllocation);
	}

	void UploadList::Clear() {
		mPendingStreamTexture = nullptr;
		mPendingLoadings.clear();
	}

}