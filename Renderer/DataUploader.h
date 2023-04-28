#pragma once
#include "DirectStorage/dstorage.h"
#include "UploadList.h"

#include "Tools/Pool.h"
#include "Tools/Ring.h"

#include <thread>
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
		struct SlotUserDataType {
		public:
			std::optional<uint64_t> uploadListIndex = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserDataType>;

	public:
		DataUploader(const GHL::Device* device, IDStorageFactory* dsFactory);
		~DataUploader();

		UploadList* AllocateUploadList();

		/*
		* 将上传任务提交到TaskSystem中
		*/
		void SubmitUploadList(UploadList* uploadList);

		inline auto* GetFileCopyQueue()   const { return mFileCopyQueue.Get(); }
		inline auto* GetMemoryCopyQueue() const { return mMemoryCopyQueue.Get(); }
		inline auto* GetCopyFence()       const { return mCopyFence.get(); }

	private:
		void MonitorThread();

	private:
		const GHL::Device* mDevice{ nullptr };
		IDStorageFactory* mDStorageFactory{ nullptr };
		Microsoft::WRL::ComPtr<IDStorageQueue> mFileCopyQueue;
		Microsoft::WRL::ComPtr<IDStorageQueue> mMemoryCopyQueue;
		
		std::unique_ptr<GHL::Fence> mCopyFence;

		// UploadList的分配池
		Pool mUploadListPool;
		std::vector<std::unique_ptr<UploadList>> mUploadLists;

		// 监视线程待处理的环形任务队列
		Tool::Ring               mMonitorTaskAlloc;
		std::vector<UploadList*> mRingMonitorTasks;
		
		// 监视线程
		bool		mThreadRunning{ false };
		HANDLE		mMonitorEvent{ nullptr };
		std::thread	mMonitorThread;	// 监视线程
	};

}