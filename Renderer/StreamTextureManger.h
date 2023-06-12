#pragma once
#include "DataUploader.h"
#include "TileUpdater.h"
#include "StreamTexture.h"

namespace GHL {
	class Device;
	class CommandQueue;
	class Fence;
}

namespace Renderer {

	class PoolDescriptorAllocator;
	class BuddyHeapAllocator;
	class RingFrameTracker;

	/*
	* 流式纹理管理器
	*/
	class StreamTextureManger {
	public:
		StreamTextureManger(
			const GHL::Device* device, 
			GHL::CommandQueue* mappingQueue,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator,
			RingFrameTracker* ringFrameTracker,
			IDStorageFactory* dstorageFactory,
			IDStorageQueue* fileCopyQueue,
			IDStorageQueue* memoryCopyQueue
			);
		~StreamTextureManger() = default;

		StreamTexture* Request(const std::string& filepath);

		void Destory();

	private:
		/*
		* 渲染帧完成后的回调函数
		*/
		void FrameCompletedCallback(uint8_t frameIndex);

	private:
		inline static const uint32_t mStagingBufferSizeMB = 128u;

		const GHL::Device* mDevice{ nullptr };
		GHL::CommandQueue* mMappingQueue{ nullptr };
		std::unique_ptr<GHL::Fence> mPackedMipMappingFence;
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		RingFrameTracker* mFrameTracker{ nullptr };
		IDStorageFactory* mDStorageFactory{ nullptr };
		std::unique_ptr<DataUploader> mDataUploader;
		std::unique_ptr<TileUpdater>  mTileUpdater;

		std::unordered_map<std::string, std::unique_ptr<StreamTexture>> mTextureStorages;
	};

}