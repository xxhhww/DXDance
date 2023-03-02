#pragma once
#include "ThreadPool.h"

namespace Core {
	/*
	* 任务系统
	*/
	class TaskSystem {
	public:
		TaskSystem(uint32_t threadCount = std::thread::hardware_concurrency() - 1);

		void Submit(std::function<void()>&& task);
	private:
		std::unique_ptr<ThreadPool> mThreadPool{ nullptr };
	};
}