#pragma once
#include <DirectStorage/dstorage.h>
#include <wrl.h>
#include <memory>

namespace GHL {
	class Device;
	class Fence;
}

namespace Renderer {
	
	class UploaderEngine {
	public:
		UploaderEngine(const GHL::Device* device);
		~UploaderEngine();

		inline auto* GetDSFactory()       const { return mDStorageFactory.Get(); }
		inline auto* GetFileCopyQueue()   const { return mFileCopyQueue.Get(); }
		inline auto* GetMemoryCopyQueue() const { return mMemoryCopyQueue.Get(); }

		inline auto* GetCopyFence()       const { return mCopyFence.get(); }
	
	private:
		inline static const uint32_t mStagingBufferSizeMB = 128u;

	private:
		const GHL::Device* mDevice{ nullptr };

		Microsoft::WRL::ComPtr<IDStorageFactory> mDStorageFactory;
		Microsoft::WRL::ComPtr<IDStorageQueue> mFileCopyQueue;
		Microsoft::WRL::ComPtr<IDStorageQueue> mMemoryCopyQueue;

		std::unique_ptr<GHL::Fence> mCopyFence;
	};

}