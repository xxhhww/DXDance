#pragma once

#include <memory>
#include <wrl.h>
#include "DirectStorage/dstorage.h"

#include "GHL/Fence.h"

namespace Renderer {

	/*
	* 数据加载器，负责将流式数据从磁盘加载到显存中
	*/
	class DataUploader {
	public:
		DataUploader(const GHL::Device* device);
		~DataUploader() = default;

		inline auto* GetDStorageFactory() const { return mDStorageFactory.Get(); }
		inline auto* GetFileCopyQueue()   const { return mFileCopyQueue.Get(); }
		inline auto* GetMemoryCopyQueue() const { return mMemoryCopyQueue.Get(); }
		inline auto* GetCopyFence()       const { return mCopyFence.get(); }

	private:
		inline static const uint32_t mStagingBufferSizeMB = 128u;

		const GHL::Device* mDevice{ nullptr };
		Microsoft::WRL::ComPtr<IDStorageFactory> mDStorageFactory;
		Microsoft::WRL::ComPtr<IDStorageQueue>   mFileCopyQueue;
		Microsoft::WRL::ComPtr<IDStorageQueue>   mMemoryCopyQueue;
		
		std::unique_ptr<GHL::Fence> mCopyFence;
	};

}