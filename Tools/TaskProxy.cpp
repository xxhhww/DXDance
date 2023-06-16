#include "TaskProxy.h"
#include "TaskSystem.h"

#include "Assert.h"

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
		, mCompletedTaskCount(0) {
		mAllCompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mAllCompletedEvent != nullptr, "Failed to Create Event Handle");
	}

	void TaskProxy::AddTask(std::function<void()>&& task) {
		mTasks.emplace_back(this, std::forward<std::function<void()>>(task));
		mTaskCount++;
	}

	void TaskProxy::RunAllTask() {
		for (auto& t : mTasks) {
			TaskSystem::GetInstance()->Submit(std::bind(&Task::Run, &t));
		}
		WaitForSingleObject(mAllCompletedEvent, INFINITE);
	}

	void TaskProxy::RunAllTaskInCurrentThread() {
		for (auto& t : mTasks) {
			t.Run();
		}
	}

	void TaskProxy::TaskCompleted() {
		// ԭ�Ӳ�����������©
		mCompletedTaskCount++;
		if (mCompletedTaskCount == mTaskCount) {
			// ֪ͨ�ȴ��̣߳�������ȫ�����
			SetEvent(mAllCompletedEvent);
		}
	}
}