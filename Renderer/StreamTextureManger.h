#pragma once
#include "DataUploader.h"
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
			RingFrameTracker* ringFrameTracker
			);
		~StreamTextureManger() = default;

		StreamTexture* Request(const std::string& filepath);

	private:
		inline static const uint32_t mStagingBufferSizeMB = 128u;

		const GHL::Device* mDevice{ nullptr };
		GHL::CommandQueue* mMappingQueue{ nullptr };
		std::unique_ptr<GHL::Fence> mMappingFence;
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		RingFrameTracker* mFrameTracker{ nullptr };
		Microsoft::WRL::ComPtr<IDStorageFactory> mDStorageFactory;
		std::unique_ptr<DataUploader> mDataUploader{ nullptr };

		std::unordered_map<std::string, std::unique_ptr<StreamTexture>> mTextureStorages;
	};

}