#include "StreamTextureManger.h"
#include "FileHandle.h"

#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"

#include "Tools/Assert.h"

namespace Renderer {

	StreamTextureManger::StreamTextureManger(
		const GHL::Device* device, 
		GHL::CommandQueue* mappingQueue,
		PoolDescriptorAllocator* descriptorAllocator,
		BuddyHeapAllocator* heapAllocator,
		RingFrameTracker* ringFrameTracker
	)
	: mDevice(device)
	, mMappingQueue(mappingQueue)
	, mMappingFence(std::make_unique<GHL::Fence>(mDevice))
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator)
	, mFrameTracker(ringFrameTracker) {
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

		// Init DataUploader
		mDataUploader = std::make_unique<DataUploader>(mDevice, mDStorageFactory.Get());
	}

	Texture* StreamTextureManger::Request(const std::string& filepath) {
		auto it = mTextureStorages.find(filepath);
		if (it != mTextureStorages.end()) {
			return it->second->GetInternalResource();
		}

		XeTexureFormat xeTextureFormat{ filepath };
		std::unique_ptr<FileHandle> fileHandle = std::make_unique<FileHandle>(mDStorageFactory.Get(), filepath);
		std::unique_ptr<StreamTexture> streamTexture = std::make_unique<StreamTexture>(
			mDevice, xeTextureFormat, std::move(fileHandle), 
			mDescriptorAllocator, mHeapAllocator, mFrameTracker);
		streamTexture->MapAndLoadPackedMipMap(mMappingQueue, mMappingFence.get(), mDataUploader->GetFileCopyQueue(), mDataUploader->GetCopyFence());
		mTextureStorages[filepath] = std::move(streamTexture);

		return mTextureStorages.at(filepath)->GetInternalResource();
	}

}