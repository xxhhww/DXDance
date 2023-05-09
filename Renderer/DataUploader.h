#pragma once
#include <DirectStorage/dstorage.h>
#include "UploadList.h"

#include "Tools/Pool.h"
#include "Tools/Ring.h"
#include "Tools/Wrap.h"

#include <thread>
#include <memory>
#include <mutex>
#include <wrl.h>

namespace GHL {
	class Device;
	class Fence;
	class CommandQueue;
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
		using UploadListWrap = Tool::Wrap<UploadList>;

	public:
		DataUploader(
			const GHL::Device* device, 
			GHL::CommandQueue* mappingQueue,
			IDStorageFactory* dsFactory);
		~DataUploader();

		/*
		* 分配一个UploadList，由ProcessFeedback线程调用
		*/
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
		GHL::CommandQueue* mMappingQueue{ nullptr };
		std::unique_ptr<GHL::Fence>	mMappingFence;
		std::mutex mMappingFenceMutex;

		IDStorageFactory* mDStorageFactory{ nullptr };
		Microsoft::WRL::ComPtr<IDStorageQueue> mFileCopyQueue;
		Microsoft::WRL::ComPtr<IDStorageQueue> mMemoryCopyQueue;

		std::unique_ptr<GHL::Fence> mCopyFence;
		std::mutex mCopyFenceMutex;

		// UploadList的分配池
		std::mutex mUploadListPoolMutex;	// TODO锁可以不需要
		Pool mUploadListPool;
		std::vector<std::unique_ptr<UploadList>> mUploadLists;

		// 监视线程待处理的环形任务队列
		Tool::Ring                  mRingMonitorTaskAlloc;
		std::vector<UploadListWrap> mRingMonitorTasks;
		
		// 监视线程
		bool		mThreadRunning{ true };
		HANDLE		mMonitorEvent{ nullptr };
		std::thread	mMonitorThread;	// 监视线程
	};

}