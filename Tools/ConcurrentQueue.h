#pragma once
#include <queue>
#include <mutex>
#include <condition_variable>

namespace Tool {
	template<typename T>
	class ConcurrentQueue {
	public:
		ConcurrentQueue() = default;
		~ConcurrentQueue() = default;

		ConcurrentQueue(ConcurrentQueue const&) = delete;
		ConcurrentQueue(ConcurrentQueue&&) = delete;

		ConcurrentQueue& operator=(ConcurrentQueue const&) = delete;
		ConcurrentQueue& operator=(ConcurrentQueue&&) = delete;

        void Push(T const& value);

        void Push(T&& value);

        void WaitPop(T& value);

		bool TryPop(T& value);

		bool Empty();

		size_t Size();

	private:
		std::queue<T>			mQueue;
		std::mutex				mMutex;
		std::condition_variable mCv;
	};
}

#include "ConcurrentQueue.inl"