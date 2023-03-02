#include "ThreadPool.h"

namespace Core {
	ThreadPool::ThreadPool(uint32_t threadSize) 
	: mOver(false) {
		static const uint32_t maxThreadSize = std::thread::hardware_concurrency();
		uint16_t const numThreads = threadSize == 0 ? maxThreadSize - 1 : (std::min)(maxThreadSize - 1, threadSize);

		mThreads.reserve(numThreads);
		for (uint16_t i = 0; i < numThreads; ++i) {
			mThreads.emplace_back(std::bind(&ThreadPool::ThreadWork, this));
		}
	}

	ThreadPool::~ThreadPool() {
		if (!mOver) {
			{
				std::unique_lock<std::mutex> lk(mCvMutex);
				mOver = true;
				mCv.notify_all();
			}
			for (int i = 0; i < mThreads.size(); ++i) {
				if (mThreads[i].joinable()) {
					mThreads[i].join();
				}
			}
			mOver = true;
		}
	}

	void ThreadPool::Submit(std::function<void()>&& task) {
		mQueue.Push(std::forward<std::function<void()>>(task));
		mCv.notify_one();
	}

	void ThreadPool::ThreadWork() {
		std::function<void()> task;
		bool popSuccess;

		while (true) {
			{
				std::unique_lock<std::mutex> lk(mCvMutex);
				while (!mOver && mQueue.Empty()) mCv.wait(lk);
				if (mOver) return;
				else popSuccess = mQueue.TryPop(task);
			}

			if (popSuccess) {
				task();
				task = nullptr;
			}
		}
	}
}