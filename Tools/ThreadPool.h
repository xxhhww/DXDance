#pragma once
#include "Tools/ConcurrentQueue.h"
#include <functional>
#include <thread>

namespace Tool {
	/*
	* �̳߳�
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
		* ��������ύ����
		*/
		void Submit(std::function<void()>&& task);
	private:
		/*
		* �����߳�ִ�еĺ���
		*/
		void ThreadWork();

	private:
		std::vector<std::thread>						mThreads;	// �߳�����
		Tool::ConcurrentQueue<std::function<void()>>	mQueue;		// �������
		bool											mOver;		// �Ƿ����
		std::mutex										mCvMutex;	// ���������õ���
		std::condition_variable							mCv;		// ��������
	};
}