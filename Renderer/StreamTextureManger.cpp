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
		RingFrameTracker* ringFrameTracker,
		IDStorageFactory* dstorageFactory,
		IDStorageQueue* fileCopyQueue,
		IDStorageQueue* memoryCopyQueue
	)
	: mDevice(device)
	, mMappingQueue(mappingQueue)
	, mPackedMipMappingFence(std::make_unique<GHL::Fence>(mDevice))
	, mDescriptorAllocator(descriptorAllocator)
	, mHeapAllocator(heapAllocator)
	, mFrameTracker(ringFrameTracker)
	, mDStorageFactory(dstorageFactory) {
		// Init DataUploader
		mDataUploader = std::make_unique<DataUploader>(mDevice, mMappingQueue, mDStorageFactory, fileCopyQueue, memoryCopyQueue);

		// Init TileUpdater
		mTileUpdater = std::make_unique<TileUpdater>(mDevice, mFrameTracker, &mTextureStorages, mDataUploader.get());

		// 注册渲染帧完成的回调函数
		mFrameTracker->AddFrameCompletedCallBack([this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
			FrameCompletedCallback(attribute.frameIndex);
		});
	}

	StreamTexture* StreamTextureManger::Request(const std::string& filepath) {
		auto it = mTextureStorages.find(filepath);
		if (it != mTextureStorages.end()) {
			return it->second.get();
		}

		XeTexureFormat xeTextureFormat{ filepath };
		std::unique_ptr<FileHandle> fileHandle = std::make_unique<FileHandle>(mDStorageFactory, filepath);
		std::unique_ptr<StreamTexture> streamTexture = std::make_unique<StreamTexture>(
			mDevice, 
			mDataUploader.get(),
			xeTextureFormat, 
			std::move(fileHandle), 
			mDescriptorAllocator, 
			mHeapAllocator, 
			mFrameTracker);
		streamTexture->MapAndLoadPackedMipMap(mMappingQueue, mPackedMipMappingFence.get(), mDataUploader->GetFileCopyQueue(), mDataUploader->GetCopyFence());
		mTextureStorages[filepath] = std::move(streamTexture);

		return mTextureStorages.at(filepath).get();
	}

	void StreamTextureManger::Destory() {
		mTextureStorages.clear();
	}

	void StreamTextureManger::FrameCompletedCallback(uint8_t frameIndex) {
		for (auto& pair : mTextureStorages) {
			pair.second->FrameCompletedCallback(frameIndex);
		}
		mTileUpdater->SetFrameCompletedEvent();
	}

}