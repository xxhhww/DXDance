#include "TaskProxy.h"
#include "TaskSystem.h"

namespace Tool {

	Task::Task(TaskProxy* proxy, std::function<void()>&& task)
		: mProxy(proxy)
		, mTaskFunc(std::move(task)) {}

	void Task::Run() {
		// 执行任务
		mTaskFunc();
		// 通知代理一个任务已经完成
		mProxy->TaskCompleted();
	}

	TaskProxy::TaskProxy(const std::string& name)
		: mName(name)
		, mTaskCount(0u)
		, mCompletedTaskCount(0) {}

	void TaskProxy::AddTask(std::function<void()>&& task) {
		mTasks.emplace_back(this, std::forward<std::function<void()>>(task));
		mTaskCount++;
	}

	void TaskProxy::RunAllTask() {
		for (auto& t : mTasks) {
			TaskSystem::GetInstance()->Submit(std::bind(&Task::Run, &t));
		}
		{
			std::unique_lock<std::mutex> lk(mCvMutex);
			mCv.wait(lk);
		}
	}

	void TaskProxy::TaskCompleted() {
		// 原子操作，不会遗漏
		mCompletedTaskCount++;
		if (mCompletedTaskCount == mTaskCount) {
			// 通知等待线程，任务已全部完成
			mCv.notify_all();
		}
	}
}