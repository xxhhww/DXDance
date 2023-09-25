#include "Renderer/StreamVirtualTextureSystem.h"

namespace Renderer {

	StreamVirtualTextureSystem::StreamVirtualTextureSystem(
		const GHL::Device* device,
		const GHL::Fence* mainFrameFence,
		const GHL::DirectStorageFactory* dStorageFactory,
		const GHL::DirectStorageQueue* dStorageFileCopyQueue,
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

		mProcessThread = std::thread([this]() {
			this->ProcessThread();
		});
	}

	StreamVirtualTextureSystem::~StreamVirtualTextureSystem() {
		mThreadRunning = false;
		mProcessThread.join();
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

				// 遍历每一个Virtual Texture处理它们的Feedback
				// 并获得对应的Task
				for (auto& vt : mStreamVirtualTextures) {

				}
			}

			// 让TaskProcessor处理Task
		}

	}
}