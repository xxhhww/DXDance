#pragma once
#include "Renderer/StreamVirtualTexture.h"
#include "Renderer/TileUploader.h"
#include "Renderer/PoolDescriptorAllocator.h"
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/RingFrameTracker.h"
#include "GHL/DirectStorageQueue.h"
#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"

#include <memory>

namespace Renderer 
{

	class StreamVirtualTextureSystem {
	public:
		StreamVirtualTextureSystem(
			const GHL::Device* device,
			const GHL::Fence* mainFrameFence,
			const GHL::DirectStorageFactory* dStorageFactory,
			const GHL::DirectStorageQueue* dStorageFileCopyQueue,
			Renderer::PoolDescriptorAllocator* mainPoolDescriptorAllocator,
			Renderer::BuddyHeapAllocator* mainBuddyHeapAllocator,
			Renderer::RingFrameTracker* mainRingFrameTracker
		);

		~StreamVirtualTextureSystem();

	private:
		// 初始化图形对象
		void InitializeGraphicsObject();

		// 处理线程
		void ProcessThread();
	private:
		// Graphics Object
		const GHL::Device* mDevice{ nullptr };
		const GHL::Fence* mMainFrameFence{ nullptr };
		const GHL::DirectStorageFactory* mDStorageFactory{ nullptr };
		const GHL::DirectStorageQueue* mDStorageFileCopyQueue{ nullptr };

		Renderer::PoolDescriptorAllocator* mMainPoolDescriptorAllocator{ nullptr };
		Renderer::BuddyHeapAllocator* mMainBuddyHeapAllocator{ nullptr };
		Renderer::RingFrameTracker* mMainRingFrameTracker{ nullptr };

		std::unique_ptr<GHL::Fence> mPackedMipMappingFence;
		std::unique_ptr<GHL::Fence> mFileCopyFence;			// 标记数据上传操作
		std::unique_ptr<GHL::CommandQueue> mMappingQueue;
		std::unique_ptr<GHL::Fence> mMappingFence;			// 标记显存映射操作

		// 纹理数组
		std::vector<std::unique_ptr<StreamVirtualTexture>> mStreamVirtualTextures;

		// Thread Parameters
		bool mThreadRunning{ true };
		std::thread mProcessThread;
		std::mutex mMutex;

		// Tile Uploader
		std::unique_ptr<TileUploader> mTileUploader;
	};

}