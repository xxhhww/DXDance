#pragma once
#include "DirectStorage/dstorage.h"
#include "UploadList.h"

#include "Tools/Pool.h"
#include "Tools/Ring.h"

#include <thread>
#include <memory>
#include <wrl.h>

namespace GHL {
	class Device;
	class Fence;
}

namespace Renderer {

	/*
	* ���ݼ�������������ʽ���ݴӴ��̼��ص��Դ���
	*/
	class DataUploader {
	public:
		struct SlotUserDataType {
		public:
			std::optional<uint64_t> uploadListIndex = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserDataType>;

	public:
		DataUploader(const GHL::Device* device, IDStorageFactory* dsFactory);
		~DataUploader();

		UploadList* AllocateUploadList();

		/*
		* ���ϴ������ύ��TaskSystem��
		*/
		void SubmitUploadList(UploadList* uploadList);

		inline auto* GetFileCopyQueue()   const { return mFileCopyQueue.Get(); }
		inline auto* GetMemoryCopyQueue() const { return mMemoryCopyQueue.Get(); }
		inline auto* GetCopyFence()       const { return mCopyFence.get(); }

	private:
		void MonitorThread();

	private:
		const GHL::Device* mDevice{ nullptr };
		IDStorageFactory* mDStorageFactory{ nullptr };
		Microsoft::WRL::ComPtr<IDStorageQueue> mFileCopyQueue;
		Microsoft::WRL::ComPtr<IDStorageQueue> mMemoryCopyQueue;
		
		std::unique_ptr<GHL::Fence> mCopyFence;

		// UploadList�ķ����
		Pool mUploadListPool;
		std::vector<std::unique_ptr<UploadList>> mUploadLists;

		// �����̴߳�����Ļ����������
		Tool::Ring               mMonitorTaskAlloc;
		std::vector<UploadList*> mRingMonitorTasks;
		
		// �����߳�
		bool		mThreadRunning{ false };
		HANDLE		mMonitorEvent{ nullptr };
		std::thread	mMonitorThread;	// �����߳�
	};

}