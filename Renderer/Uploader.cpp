#include "Uploader.h"
#include "Tools/Assert.h"

namespace Renderer {

	Uploader::Uploader(GHL::CopyQueue* copyQueue, uint32_t maxUploadBatch)
	: mCopyQueue(copyQueue) 
	, mSubmitTaskAlloc(maxUploadBatch)
	, mMonitorTaskAlloc(maxUploadBatch) 
	, mCopyFence(mCopyQueue->GetDevice())
	, mRingSubmitTasks(maxUploadBatch)
	, mRingMonitorTasks(maxUploadBatch) {}

	Uploader::~Uploader() {

	}

	void Uploader::StartThreads() {
		ASSERT_FORMAT(mThreadRunning == false, "Start Thread Failed");
		mThreadRunning = true;

		mSubmitThread = std::thread([this]() {
			while (mThreadRunning) {
				std::unique_lock lck(mSubmitMutex);
				mSubmitCV.wait(lck);
				SubmitThread();
			}
		});

		mMonitorThread = std::thread([this]() {
			while (mThreadRunning) {
				if (mUploadListPool.AllocatedSize() == 0u) {
					std::unique_lock lck(mMonitorMutex);
					mMonitorCV.wait(lck);
				}
				MonitorThread();
			}
		});

	}

	void Uploader::StopThreads() {

	}

	UploadList* Uploader::AllocateUploadList() {
		auto* slot = mUploadListPool.Allocate();
		if (!slot->userData.uploadistIndex) {
			mUploadLists.emplace_back(std::make_unique<UploadList>());
			slot->userData.uploadistIndex = mUploadLists.size() - 1u;
		}

		UploadList* targetList = mUploadLists.at(*slot->userData.uploadistIndex).get();
		targetList->mUploadState = UploadList::State::Allocated;

		// 通知监视线程
		mRingMonitorTasks[mMonitorTaskAlloc.GetWriteIndex()] = targetList;
		mMonitorTaskAlloc.Allocate();
		mMonitorCV.notify_one();

		return mUploadLists.at(*slot->userData.uploadistIndex).get();
	}

	void Uploader::SubmitUploadList(UploadList* uploadList) {
		uploadList->SetUploadState(UploadList::State::Submitted);

		// 通知执行线程
		mRingSubmitTasks[mSubmitTaskAlloc.GetWriteIndex()] = uploadList;
		mSubmitTaskAlloc.Allocate();
		mSubmitCV.notify_one();
	}

	void Uploader::DeallocateUploadList(UploadList* uploadList) {

	}

	void Uploader::SubmitThread() {
		
		bool needSignal{ false };

		// look through tasks
		while (mSubmitTaskAlloc.GetReadyToRead()) {
			auto* updateList = mRingSubmitTasks[mSubmitTaskAlloc.GetReadIndex()]; // get the next task
			mSubmitTaskAlloc.Free(); // consume this task

			updateList->SetExpectedFenceValue(mCopyFence.ExpectedValue());

			if (updateList->IsDataUploadTask()) {

			}
			else {

			}

			updateList->SetUploadState(UploadList::State::Processing);
			
			needSignal = true;
		}

		if (needSignal) {
			mCopyQueue->SignalFence(mCopyFence);
			mCopyFence.IncrementExpectedValue();
		}

	}

	void Uploader::MonitorThread() {
		const uint32_t numTasks = mMonitorTaskAlloc.GetReadyToRead();
		if (numTasks == 0u) {
			return;
		}

		const uint32_t startIndex = mMonitorTaskAlloc.GetReadIndex();
		for (size_t i = startIndex; i < (startIndex + numTasks); i++) {

			auto* uploadList = mRingMonitorTasks[i];

			bool freeUpdateList{ false };

			// GPU完成uploadList塞入的命令
			if (mCopyFence.CompletedValue() >= uploadList->GetExpectedFenceValue()) {



				freeUpdateList = true;
			}

			if (freeUpdateList) {

			}

		}
	}
}