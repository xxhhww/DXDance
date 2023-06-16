#pragma once
#pragma once
#include <vector>
#include <functional>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>

#include <Windows.h>

namespace Tool {
	class TaskSystem;
	class TaskProxy;

	/*
	* ����
	*/
	class Task {
		friend class TaskProxy;
	public:
		/*
		* ���캯��
		*/
		Task(TaskProxy* proxy, std::function<void()>&& task);

	private:
		/*
		* �������к���
		*/
		void Run();
	private:
		TaskProxy* mProxy{ nullptr };		// ������������
		std::function<void()>	mTaskFunc{ nullptr };	// ִ�з���
	};

	/*
	* �������
	*/
	class TaskProxy {
		friend class TaskSystem;
		friend class Task;
	public:
		/*
		* ���캯��
		*/
		TaskProxy(const std::string& name = "");

		/*
		* ����µ�����
		*/
		void AddTask(std::function<void()>&& task);

		/*
		* ͬ���ȴ������������
		*/
		void RunAllTask();

		/*
		* �ڵ�ǰ�߳�������ȫ������
		*/
		void RunAllTaskInCurrentThread();

	private:
		/*
		* һ���������ʱ�����ĺ���
		*/
		void TaskCompleted();
	private:
		std::string				mName;
		std::vector<Task>		mTasks;
		uint32_t				mTaskCount{ 0u };			// ��������
		std::atomic<uint32_t>	mCompletedTaskCount{ 0 };	// ����ɵ��������
		HANDLE                  mAllCompletedEvent{ nullptr };
	};
}