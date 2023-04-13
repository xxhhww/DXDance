#include "TaskSystem.h"

namespace Tool {
	TaskSystem::TaskSystem(uint32_t threadCount) {
		mThreadPool = std::make_unique<ThreadPool>(threadCount);
	}

	void TaskSystem::Submit(std::function<void()>&& task) {
		mThreadPool->Submit(std::forward<std::function<void()>>(task));
	}

	TaskSystem* TaskSystem::GetInstance() {
		static TaskSystem _sTaskSystem;
		return &_sTaskSystem;
	}

}