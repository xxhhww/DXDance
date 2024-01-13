#include "GHL/DirectStorageFactory.h"
#include "Tools/Assert.h"

namespace GHL {

	DirectStorageFactory::DirectStorageFactory(size_t stagingBufferSizeMB) {
		// Init DStorage
		DSTORAGE_CONFIGURATION dsConfig{};
		DStorageSetConfiguration(&dsConfig);

		HRASSERT(DStorageGetFactory(IID_PPV_ARGS(&mDStorageFactory)));
		DSTORAGE_DEBUG debugFlags = DSTORAGE_DEBUG_NONE;
#ifdef _DEBUG
		debugFlags = DSTORAGE_DEBUG_SHOW_ERRORS;
#endif
		mDStorageFactory->SetDebugFlags(debugFlags);
		mDStorageFactory->SetStagingBufferSize(stagingBufferSizeMB * 1024u * 1024u);
	}

}