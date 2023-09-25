#include "Renderer/TileUploader.h"

namespace Renderer {

	TileUploader::TileUploader(
		const GHL::DirectStorageQueue* dStorageFileCopyQueue,
		const GHL::Fence* fileCopyFence,
		const GHL::CommandQueue* mappingQueue,
		const GHL::Fence* mappingFence
	) 
	: mDStorageFileCopyQueue(dStorageFileCopyQueue)
	, mFileCopyFence(fileCopyFence)
	, mMappingQueue(mappingQueue)
	, mMappingFence(mappingFence) 
	, mRingMonitorTasks(512)
	, mRingMonitorTaskAllocator(512) {

		// 启动监视线程
		mMonitorEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mMonitorEvent != nullptr, "Failed to Create Monitor Event Handle");
		mMonitorThread = std::thread([this]() {
			MonitorThread();
		});
	}

	TileUploader::~TileUploader() {
		mThreadRunning = false;
		SetEvent(mMonitorEvent);
		mMonitorThread.join();
	}

	TileUploader::Task* TileUploader::AllocateTask() {
		Pool::Slot* slot = mTaskPoolAllocator.Allocate();

		if (!slot->userData.taskIndex) {
			mTasks.emplace_back(std::make_unique<Task>());
			slot->userData.taskIndex = mTasks.size() - 1u;
		}

		Task* task = mTasks.at(*slot->userData.taskIndex).get();
		task->Reset();
		task->SetUploadListState(TileUploader::Task::State::Allocated);

		// 通知监视线程，开始监视UploadList
		mRingMonitorTasks[mRingMonitorTaskAllocator.GetWriteIndex()] = task;
		mRingMonitorTaskAllocator.Allocate();
		SetEvent(mMonitorEvent);

		return mTasks.at(*slot->userData.taskIndex).get();
	}

	void TileUploader::SubmitTask(Task* task) {

	}

	void TileUploader::MonitorThread() {

	}

}