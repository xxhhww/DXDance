#pragma once
#include "Tools/ConcurrentQueue.h"
#include <functional>
#include <thread>

namespace Tool {
	/*
	* 线程池
	*/
	class ThreadPool {
	public:
		ThreadPool(uint32_t pool_size = std::thread::hardware_concurrency() - 1);
		~ThreadPool();

		ThreadPool(ThreadPool const&) = delete;
		ThreadPool(ThreadPool&&) = delete;

		ThreadPool& operator=(ThreadPool const&) = delete;
		ThreadPool& operator=(ThreadPool&&) = delete;

		/*
		* 向队列中提交任务
		*/
		void Submit(std::function<void()>&& task);
	private:
		/*
		* 工作线程执行的函数
		*/
		void ThreadWork();

	private:
		std::vector<std::thread>						mThreads;	// 线程数组
		Tool::ConcurrentQueue<std::function<void()>>	mQueue;		// 任务队列
		bool											mOver;		// 是否结束
		std::mutex										mCvMutex;	// 条件变量用的锁
		std::condition_variable							mCv;		// 条件变量
	};
}