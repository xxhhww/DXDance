#include "GHL/DirectStorageQueue.h"
#include "Tools/Assert.h"

namespace GHL {

	DirectStorageQueue::DirectStorageQueue(const Device* device, const DirectStorageFactory* factory, DSTORAGE_REQUEST_SOURCE_TYPE requestType) 
	: mDevice(device)
	, mDStorageFactory(factory) {
		// Init DStorage Queue
		DSTORAGE_QUEUE_DESC queueDesc{};
		queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
		queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
		queueDesc.SourceType = requestType;
		queueDesc.Device = device->D3DDevice();

		HRASSERT(factory->GetDStorageFactory()->CreateQueue(&queueDesc, IID_PPV_ARGS(&mDStorageQueue)));
	}


}