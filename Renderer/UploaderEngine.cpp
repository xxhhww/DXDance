#include "UploaderEngine.h"

#include "GHL/Device.h"
#include "GHL/Fence.h"

#include "Tools/Assert.h"

namespace Renderer {

	UploaderEngine::UploaderEngine(const GHL::Device* device) 
	: mDevice(device) {
		// Init DStorage
		DSTORAGE_CONFIGURATION dsConfig{};
		DStorageSetConfiguration(&dsConfig);

		HRASSERT(DStorageGetFactory(IID_PPV_ARGS(&mDStorageFactory)));
		DSTORAGE_DEBUG debugFlags = DSTORAGE_DEBUG_NONE;
#ifdef _DEBUG
		debugFlags = DSTORAGE_DEBUG_SHOW_ERRORS;
#endif
		mDStorageFactory->SetDebugFlags(debugFlags);
		mDStorageFactory->SetStagingBufferSize(mStagingBufferSizeMB * 1024u * 1024u);
		
		// Init DStorage Queue
		DSTORAGE_QUEUE_DESC queueDesc{};
		queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
		queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
		queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
		queueDesc.Device = mDevice->D3DDevice();

		HRASSERT(mDStorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mFileCopyQueue)));

		queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
		HRASSERT(mDStorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mMemoryCopyQueue)));

		// Init Copy Fence
		mCopyFence = std::make_unique<GHL::Fence>(mDevice);
	}

	UploaderEngine::~UploaderEngine() {}

}