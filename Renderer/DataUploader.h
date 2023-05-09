#pragma once
#include <DirectStorage/dstorage.h>
#include "UploadList.h"

#include "Tools/Pool.h"
#include "Tools/Ring.h"
#include "Tools/Wrap.h"

#include <thread>
#include <memory>
#include <mutex>
#include <wrl.h>

namespace GHL {
	class Device;
	class Fence;
	class CommandQueue;
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
		using UploadListWrap = Tool::Wrap<UploadList>;

	public:
		DataUploader(
			const GHL::Device* device, 
			GHL::CommandQueue* mappingQueue,
			IDStorageFactory* dsFactory);
		~DataUploader();

		/*
		* ����һ��UploadList����ProcessFeedback�̵߳���
		*/
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
		GHL::CommandQueue* mMappingQueue{ nullptr };
		std::unique_ptr<GHL::Fence>	mMappingFence;
		std::mutex mMappingFenceMutex;

		IDStorageFactory* mDStorageFactory{ nullptr };
		Microsoft::WRL::ComPtr<IDStorageQueue> mFileCopyQueue;
		Microsoft::WRL::ComPtr<IDStorageQueue> mMemoryCopyQueue;

		std::unique_ptr<GHL::Fence> mCopyFence;
		std::mutex mCopyFenceMutex;

		// UploadList�ķ����
		std::mutex mUploadListPoolMutex;	// TODO�����Բ���Ҫ
		Pool mUploadListPool;
		std::vector<std::unique_ptr<UploadList>> mUploadLists;

		// �����̴߳�����Ļ����������
		Tool::Ring                  mRingMonitorTaskAlloc;
		std::vector<UploadListWrap> mRingMonitorTasks;
		
		// �����߳�
		bool		mThreadRunning{ true };
		HANDLE		mMonitorEvent{ nullptr };
		std::thread	mMonitorThread;	// �����߳�
	};

}