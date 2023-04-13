#pragma once
#pragma once
#include <vector>
#include <functional>
#include <string>
#include <atomic>
#include <mutex>
#include <condition_variable>

namespace Tool {
	class TaskSystem;
	class TaskProxy;

	/*
	* 任务
	*/
	class Task {
		friend class TaskProxy;
	public:
		/*
		* 构造函数
		*/
		Task(TaskProxy* proxy, std::function<void()>&& task);

	private:
		/*
		* 任务运行函数
		*/
		void Run();
	private:
		TaskProxy* mProxy{ nullptr };		// 任务所属代理
		std::function<void()>	mTaskFunc{ nullptr };	// 执行方法
	};

	/*
	* 任务代理
	*/
	class TaskProxy {
		friend class TaskSystem;
		friend class Task;
	public:
		/*
		* 构造函数
		*/
		TaskProxy(const std::string& name = "");

		/*
		* 添加新的任务
		*/
		void AddTask(std::function<void()>&& task);

		/*
		* 同步等待任务运行完成
		*/
		void RunAllTask();

	private:
		/*
		* 一个任务完成时触发改函数
		*/
		void TaskCompleted();
	private:
		std::string				mName;
		std::vector<Task>		mTasks;
		uint32_t				mTaskCount{ 0u };			// 任务总数
		std::atomic<uint32_t>	mCompletedTaskCount{ 0 };	// 已完成的任务个数
		std::mutex				mCvMutex;					// 条件变量的锁
		std::condition_variable mCv;						// 条件变量
	};
}