#pragma once
#include "GHL/DirectStorageFactory.h"
#include "GHL/Device.h"
namespace GHL {

	class DirectStorageQueue {
	public:
		DirectStorageQueue(const Device* device, const DirectStorageFactory* factory, DSTORAGE_REQUEST_SOURCE_TYPE requestType);

		inline IDStorageQueue* GetDStorageQueue() const { return mDStorageQueue.Get(); }
	private:
		const Device* mDevice = nullptr;
		const DirectStorageFactory* mDStorageFactory = nullptr;
		Microsoft::WRL::ComPtr<IDStorageQueue> mDStorageQueue;
	};

}