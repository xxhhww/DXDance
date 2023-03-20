#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include "UploadList.h"

#include "Tools/Pool.h"
#include "Tools/Ring.h"

#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"

namespace Renderer {

	/*
	* 资源上传器，目前设想的主要功能如下：
	* Uploader启动时应该拥有两个线程：命令提交线程 与 监视线程
	* 其中，命令提交线程负责执行UploadList中表示的任务，监视线程则负责对当前正在执行的UploadList进行监视，并在UploadList的状态发生转变时调用执行一些对应的操作(可以是回调函数)
	* Uploader其本身也负责UploadList的分配与回收工作
	*/
	class Uploader {
	public:
		struct SlotUserDataType {
		public:
			std::optional<uint64_t> uploadistIndex = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserDataType>;

	public:
		Uploader(GHL::CopyQueue* copyQueue, uint32_t maxUploadBatch = 1024u);
		~Uploader();

		void StartThreads();

		void StopThreads();

		UploadList* AllocateUploadList();

		void SubmitUploadList(UploadList* uploadList);

		void DeallocateUploadList(UploadList* uploadList);

	private:
		void SubmitThread();

		void MonitorThread();

	private:
		GHL::CopyQueue* mCopyQueue{ nullptr }; // GPU复制引擎
		GHL::Fence      mCopyFence;

		std::thread mSubmitThread;
		std::thread mMonitorThread;

		// 执行线程的条件变量
		std::mutex              mSubmitMutex;
		std::condition_variable mSubmitCV;

		// 监视线程的条件变量
		std::mutex              mMonitorMutex;
		std::condition_variable mMonitorCV;

		// UploadList的池
		Pool                                     mUploadListPool;
		std::vector<std::unique_ptr<UploadList>> mUploadLists;

		// 待处理与监督的环形任务队列
		std::vector<UploadList*> mRingSubmitTasks;
		Tool::Ring               mSubmitTaskAlloc;

		std::vector<UploadList*> mRingMonitorTasks;
		Tool::Ring               mMonitorTaskAlloc;

		std::atomic<bool>        mThreadRunning{ false };

	};

}