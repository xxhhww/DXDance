#include "Renderer/StreamVirtualTextureSystem.h"

namespace Renderer {

	StreamVirtualTextureSystem::StreamVirtualTextureSystem(
		const GHL::Device* device,
		const GHL::Fence* mainFrameFence,
		GHL::DirectStorageFactory* dStorageFactory,
		GHL::DirectStorageQueue* dStorageFileCopyQueue,
		Renderer::PoolDescriptorAllocator* mainPoolDescriptorAllocator,
		Renderer::BuddyHeapAllocator* mainBuddyHeapAllocator,
		Renderer::RingFrameTracker* mainRingFrameTracker
	) 
	: mDevice(device)
	, mMainFrameFence(mainFrameFence)
	, mDStorageFactory(dStorageFactory)
	, mDStorageFileCopyQueue(dStorageFileCopyQueue)
	, mMainPoolDescriptorAllocator(mainPoolDescriptorAllocator)
	, mMainBuddyHeapAllocator(mainBuddyHeapAllocator)
	, mMainRingFrameTracker(mainRingFrameTracker) {
		InitializeGraphicsObject();

		mTileUploader = std::make_unique<TileUploader>(dStorageFileCopyQueue, mFileCopyFence.get(), mMappingQueue.get(), mMappingFence.get());

		mProcessThread = std::thread([this]() {
			this->ProcessThread();
		});

		mResidencyChangedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mResidencyChangedEvent != nullptr, "Failed to Create Residency Changed Event Handle");
		mUpdateResidencyThread = std::thread([this]() {
			this->UpdateResidencyThread();
		});
	}

	StreamVirtualTextureSystem::~StreamVirtualTextureSystem() {
		mThreadRunning = false;
		mProcessThread.join();
		mUpdateResidencyThread.join();
	}

	void StreamVirtualTextureSystem::SetResidencyChanged() {
		SetEvent(mResidencyChangedEvent);
	}

	void StreamVirtualTextureSystem::InitializeGraphicsObject() {
		mPackedMipMappingFence = std::make_unique<GHL::Fence>(mDevice);
		mFileCopyFence = std::make_unique<GHL::Fence>(mDevice);
		mMappingQueue = std::make_unique<GHL::CommandQueue>(mDevice, D3D12_COMMAND_LIST_TYPE_COPY);
	}

	void StreamVirtualTextureSystem::ProcessThread() {

		uint64_t previousMainFrameFenceValue = 0u;
		bool moreTask = false;

		while (mThreadRunning) {

			uint64_t currentMainFrameFenceValue = mMainFrameFence->CompletedValue();

			// 新的主渲染帧完成
			if (previousMainFrameFenceValue != currentMainFrameFenceValue) {
				previousMainFrameFenceValue = currentMainFrameFenceValue;

				// 遍历每一个Stream Virtual Texture处理它们的Feedback，收集TileLoadings TileEvictions
				for (auto& svt : mStreamVirtualTextures) {
					svt->ProcessFeedback(currentMainFrameFenceValue);
					moreTask = true;
				}
			}

			if (moreTask) {
				moreTask = false;
				uint32_t numLoadings = 0u;
				uint32_t numEvictions = 0u;
				for (auto& svt : mStreamVirtualTextures) {
					if (svt->IsStale()) {
						// 处理PendingTileLoadings和PendingTileEvictions

						// 如果在处理过程中，检测到渲染主线程的一帧被完成，则需要放弃后续处理，优先处理ProcessFeedback
						if (mMainFrameFence->CompletedValue() == previousMainFrameFenceValue) {
							numLoadings += svt->ProcessTileLoadings();
						}

						numEvictions += svt->ProcessTileEvictions();
					}

					// 如果在Pending处理完之后还有任务，则设置moreTask
					if (svt->IsStale()) {
						moreTask = true;
					}
				}

				// 压入GPU Task
				if (numLoadings) {
					mTileUploader->SignalGPUTask();
				}
			}

		}

	}

	// 更新驻留信息线程
	void StreamVirtualTextureSystem::UpdateResidencyThread() {
		while (mThreadRunning) {
			WaitForSingleObject(mResidencyChangedEvent, INFINITE);
			if (!mThreadRunning) break;

			for (auto& svt : mStreamVirtualTextures) {
				svt->UpdateMinMipMap();
			}
		}
	}

}