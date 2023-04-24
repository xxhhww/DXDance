#include "DataUploader.h"

#include "GHL/Device.h"
#include "GHL/Fence.h"

#include "Tools/Assert.h"

namespace Renderer {
	DataUploader::DataUploader(const GHL::Device* device, IDStorageFactory* dsFactory)
    : mDevice(device)
    , mDStorageFactory(dsFactory)
	, mCopyFence(std::make_unique<GHL::Fence>(device)) {
        DSTORAGE_QUEUE_DESC queueDesc{};
        queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
        queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        queueDesc.Device = device->D3DDevice();

        HRASSERT(mDStorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mFileCopyQueue)));

        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
        HRASSERT(mDStorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mMemoryCopyQueue)));
	}
}