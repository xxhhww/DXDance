#include "DataUploader.h"

#include "Tools/Assert.h"

namespace Renderer {
	DataUploader::DataUploader(const GHL::Device* device) 
	: mCopyFence(std::make_unique<GHL::Fence>(device)) {

        // initialize to default values
        DSTORAGE_CONFIGURATION dsConfig{};
        DStorageSetConfiguration(&dsConfig);

        HRASSERT(DStorageGetFactory(IID_PPV_ARGS(&mDStorageFactory)));
        DSTORAGE_DEBUG debugFlags = DSTORAGE_DEBUG_NONE;
#ifdef _DEBUG
        debugFlags = DSTORAGE_DEBUG_SHOW_ERRORS;
#endif
        mDStorageFactory->SetDebugFlags(debugFlags);
        mDStorageFactory->SetStagingBufferSize(mStagingBufferSizeMB * 1024u * 1024u);

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