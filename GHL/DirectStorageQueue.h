#pragma once
#include "GHL/DirectStorageFactory.h"
#include "GHL/Device.h"
#include "GHL/Fence.h"

namespace GHL {

	class DirectStorageQueue {
	public:
		DirectStorageQueue(const Device* device, const DirectStorageFactory* factory, DSTORAGE_REQUEST_SOURCE_TYPE requestType);

		void EnqueueRequest(const DSTORAGE_REQUEST* request);

		void EnqueueSignal(const Fence& fence, std::optional<uint64_t> expectedValue = std::nullopt);

		void Submit();

		inline IDStorageQueue* GetDStorageQueue() const { return mDStorageQueue.Get(); }

	private:
		const Device* mDevice = nullptr;
		const DirectStorageFactory* mDStorageFactory = nullptr;
		Microsoft::WRL::ComPtr<IDStorageQueue> mDStorageQueue;
	};

}