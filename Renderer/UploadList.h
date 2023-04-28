#pragma once
#include "GHL/pbh.h"
#include "BuddyHeapAllocator.h"
#include <atomic>

namespace Renderer {

	class StreamTexture;
	class DataUploader;

	class UploadList {
		friend class DataUploader;
	public:
		enum class State {
			Free,
			Allocated,
			Processing,
		};

	public:
		UploadList();
		~UploadList() = default;

		void SetUploadListState(UploadList::State state);

		void SetPendingStreamTexture(StreamTexture* pendingTexture);

		void PushPendingLoadings(
			const D3D12_TILED_RESOURCE_COORDINATE& coord,
			BuddyHeapAllocator::Allocation* heapAllocation);

		void Clear();

	private:
		std::atomic<UploadList::State> mUploadState{ State::Free };

		StreamTexture* mPendingStreamTexture{ nullptr };
		std::vector<D3D12_TILED_RESOURCE_COORDINATE> mPendingLoadings;
		std::vector<BuddyHeapAllocator::Allocation*> mHeapAllocations;
	};

}