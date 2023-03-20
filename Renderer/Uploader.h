#pragma once
#include <thread>
#include <mutex>
#include <condition_variable>
#include <optional>
#include "UploadList.h"

#include "Tools/Pool.h"
#include "Tools/Ring.h"

#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"

namespace Renderer {

	/*
	* ��Դ�ϴ�����Ŀǰ�������Ҫ�������£�
	* Uploader����ʱӦ��ӵ�������̣߳������ύ�߳� �� �����߳�
	* ���У������ύ�̸߳���ִ��UploadList�б�ʾ�����񣬼����߳�����Ե�ǰ����ִ�е�UploadList���м��ӣ�����UploadList��״̬����ת��ʱ����ִ��һЩ��Ӧ�Ĳ���(�����ǻص�����)
	* Uploader�䱾��Ҳ����UploadList�ķ�������չ���
	*/
	class Uploader {
	public:
		struct SlotUserDataType {
		public:
			std::optional<uint64_t> uploadistIndex = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserDataType>;

	public:
		Uploader(GHL::CopyQueue* copyQueue, uint32_t maxUploadBatch = 1024u);
		~Uploader();

		void StartThreads();

		void StopThreads();

		UploadList* AllocateUploadList();

		void SubmitUploadList(UploadList* uploadList);

		void DeallocateUploadList(UploadList* uploadList);

	private:
		void SubmitThread();

		void MonitorThread();

	private:
		GHL::CopyQueue* mCopyQueue{ nullptr }; // GPU��������
		GHL::Fence      mCopyFence;

		std::thread mSubmitThread;
		std::thread mMonitorThread;

		// ִ���̵߳���������
		std::mutex              mSubmitMutex;
		std::condition_variable mSubmitCV;

		// �����̵߳���������
		std::mutex              mMonitorMutex;
		std::condition_variable mMonitorCV;

		// UploadList�ĳ�
		Pool                                     mUploadListPool;
		std::vector<std::unique_ptr<UploadList>> mUploadLists;

		// ��������ල�Ļ����������
		std::vector<UploadList*> mRingSubmitTasks;
		Tool::Ring               mSubmitTaskAlloc;

		std::vector<UploadList*> mRingMonitorTasks;
		Tool::Ring               mMonitorTaskAlloc;

		std::atomic<bool>        mThreadRunning{ false };

	};

}