#include "DataUploader.h"

#include "GHL/Device.h"
#include "GHL/Fence.h"

#include "Tools/Assert.h"

namespace Renderer {

	DataUploader::DataUploader(const GHL::Device* device, IDStorageFactory* dsFactory)
    : mDevice(device)
    , mDStorageFactory(dsFactory)
	, mCopyFence(std::make_unique<GHL::Fence>(device)) 
    , mRingMonitorTasks(512)
    , mMonitorTaskAlloc(512) {
        DSTORAGE_QUEUE_DESC queueDesc{};
        queueDesc.Capacity = DSTORAGE_MAX_QUEUE_CAPACITY;
        queueDesc.Priority = DSTORAGE_PRIORITY_NORMAL;
        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
        queueDesc.Device = device->D3DDevice();

        HRASSERT(mDStorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mFileCopyQueue)));

        queueDesc.SourceType = DSTORAGE_REQUEST_SOURCE_MEMORY;
        HRASSERT(mDStorageFactory->CreateQueue(&queueDesc, IID_PPV_ARGS(&mMemoryCopyQueue)));

        // 初始化MonitorThread
        mMonitorEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
        ASSERT_FORMAT(mMonitorEvent != nullptr, "Failed to Create Monitor Event Handle");
        mMonitorThread = std::thread([this]() {
            MonitorThread();
        });
	}

    DataUploader::~DataUploader() {
        SetEvent(mMonitorEvent);
        mMonitorThread.join();
    }

    UploadList* DataUploader::AllocateUploadList() {
        auto* slot = mUploadListPool.Allocate();
        if (!slot->userData.uploadListIndex) {
            mUploadLists.emplace_back(std::make_unique<UploadList>());
            slot->userData.uploadListIndex = mUploadLists.size() - 1u;
        }

        UploadList* targetList = mUploadLists.at(*slot->userData.uploadListIndex).get();
        targetList->Clear();
        targetList->SetUploadListState(UploadList::State::Allocated);

        // 通知监视线程，开始监视UploadList
        mRingMonitorTasks[mMonitorTaskAlloc.GetWriteIndex()] = targetList;
        mMonitorTaskAlloc.Allocate();
        SetEvent(mMonitorEvent);

        return mUploadLists.at(*slot->userData.uploadListIndex).get();
    }

    void DataUploader::SubmitUploadList(UploadList* uploadList) {
        /*
        * 将上传任务提交到TaskSystem中
        */

    }

    void DataUploader::MonitorThread() {
        while (mThreadRunning) {
            WaitForSingleObject(mMonitorEvent, INFINITE);
        }
    }

}