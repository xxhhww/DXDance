#pragma once
#include "DirectStorage/dstorage.h"
#include <memory>
#include <wrl.h>

namespace GHL {
	class Device;
	class Fence;
}

namespace Renderer {

	/*
	* 数据加载器，负责将流式数据从磁盘加载到显存中
	*/
	class DataUploader {
	public:
		DataUploader(const GHL::Device* device, IDStorageFactory* dsFactory);
		~DataUploader() = default;

		inline auto* GetFileCopyQueue()   const { return mFileCopyQueue.Get(); }
		inline auto* GetMemoryCopyQueue() const { return mMemoryCopyQueue.Get(); }
		inline auto* GetCopyFence()       const { return mCopyFence.get(); }

	private:
		const GHL::Device* mDevice{ nullptr };
		IDStorageFactory* mDStorageFactory{ nullptr };
		Microsoft::WRL::ComPtr<IDStorageQueue> mFileCopyQueue;
		Microsoft::WRL::ComPtr<IDStorageQueue> mMemoryCopyQueue;
		
		std::unique_ptr<GHL::Fence> mCopyFence;
	};

}