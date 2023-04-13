#include "TaskProxy.h"
#include "TaskSystem.h"

namespace Tool {

	Task::Task(TaskProxy* proxy, std::function<void()>&& task)
		: mProxy(proxy)
		, mTaskFunc(std::move(task)) {}

	void Task::Run() {
		// ִ������
		mTaskFunc();
		// ֪ͨ����һ�������Ѿ����
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
		// ԭ�Ӳ�����������©
		mCompletedTaskCount++;
		if (mCompletedTaskCount == mTaskCount) {
			// ֪ͨ�ȴ��̣߳�������ȫ�����
			mCv.notify_all();
		}
	}
}