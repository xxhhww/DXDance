#include "DataUploader.h"
#include "StreamTexture.h"
#include "FileHandle.h"

#include "GHL/Device.h"
#include "GHL/Fence.h"
#include "GHL/CommandQueue.h"

#include "Tools/Assert.h"
#include "Tools/TaskSystem.h"

namespace Renderer {

	DataUploader::DataUploader(
        const GHL::Device* device,
        GHL::CommandQueue* mappingQueue,
        IDStorageFactory* dsFactory,
        IDStorageQueue* fileCopyQueue,
        IDStorageQueue* memoryCopyQueue)
    : mDevice(device)
    , mMappingQueue(mappingQueue)
    , mMappingFence(std::make_unique<GHL::Fence>(mDevice))
    , mDStorageFactory(dsFactory)
	, mCopyFence(std::make_unique<GHL::Fence>(device)) 
    , mRingMonitorTasks(512)
    , mRingMonitorTaskAlloc(512) {

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
        mThreadRunning = false;
        SetEvent(mMonitorEvent);
        mMonitorThread.join();
    }

    UploadList* DataUploader::AllocateUploadList() {
        Pool::Slot* slot = nullptr;
        {
            std::lock_guard lock(mUploadListPoolMutex);
            slot = mUploadListPool.Allocate();
        }

        if (!slot->userData.uploadListIndex) {
            mUploadLists.emplace_back(std::make_unique<UploadList>());
            slot->userData.uploadListIndex = mUploadLists.size() - 1u;
        }

        UploadList* targetList = mUploadLists.at(*slot->userData.uploadListIndex).get();
        targetList->Clear();
        targetList->SetUploadListState(UploadList::State::Allocated);

        // 通知监视线程，开始监视UploadList
        mRingMonitorTasks[mRingMonitorTaskAlloc.GetWriteIndex()] = UploadListWrap{
            targetList, 
            [this, slot]() {
                mUploadListPool.Deallocate(slot);
            }
        };
        mRingMonitorTaskAlloc.Allocate();
        SetEvent(mMonitorEvent);

        return mUploadLists.at(*slot->userData.uploadListIndex).get();
    }

    void DataUploader::SubmitUploadList(UploadList* uploadList) {

        // 数据上传任务，使用DStorage的Queue，其实还是CopyQueue
        auto mappingAndUploadTask = [this, uploadList]() {
            {
                // mapping tasks
                auto* streamTexture = uploadList->mPendingStreamTexture;

                uint32_t numCoords = uploadList->mPendingLoadings.size();
                for (uint32_t i = 0u; i < numCoords; i++) {
                    const auto& currCoord = uploadList->mPendingLoadings.at(i);
                    const auto* currAlloc = uploadList->mHeapAllocations.at(i);

                    D3D12_TILE_REGION_SIZE resourceRegionSizes{ 1u, FALSE, 0, 0, 0 };
                    std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags(1u, D3D12_TILE_RANGE_FLAG_NONE);

                    mMappingQueue->D3DCommandQueue()->UpdateTileMappings(
                        streamTexture->GetInternalResource()->D3DResource(),
                        1u,
                        &currCoord,
                        &resourceRegionSizes,
                        currAlloc->heap->D3DHeap(),
                        1u,
                        rangeFlags.data(),
                        &currAlloc->tileOffset,
                        nullptr,
                        D3D12_TILE_MAPPING_FLAG_NONE
                    );
                }

                {
                    // 可优化
                    std::lock_guard lock(mMappingFenceMutex);
                    mMappingFence->IncrementExpectedValue();
                    mMappingQueue->SignalFence(*mMappingFence);
                    uploadList->mMappingFenceValue = mMappingFence->ExpectedValue();
                }

            }

            // 数据在上传之前必须要完成对应显存的映射
            mMappingFence->Wait();

            {
                // upload tasks
                auto* streamTexture = uploadList->mPendingStreamTexture;
                const auto& fileFormat = streamTexture->GetFileFormat();
                const auto* fileHandle = streamTexture->GetFileHandle();

                DSTORAGE_REQUEST dsRequest{};
                dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
                dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TILES;
                dsRequest.Options.CompressionFormat = (DSTORAGE_COMPRESSION_FORMAT)fileFormat.GetCompressionFormat();
                dsRequest.Destination.Tiles.TileRegionSize = D3D12_TILE_REGION_SIZE{ 1, FALSE, 0, 0, 0 };
                dsRequest.Source.File.Source = fileHandle->GetDStorageFile();
                dsRequest.UncompressedSize = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;

                uint32_t numCoords = uploadList->mPendingLoadings.size();
                for (uint32_t i = 0u; i < numCoords; i++) {
                    const auto& currCoord = uploadList->mPendingLoadings.at(i);
                    auto fileOffset = fileFormat.GetFileOffset(currCoord);

                    dsRequest.Source.File.Offset = fileOffset.offset;
                    dsRequest.Source.File.Size = fileOffset.numBytes;

                    dsRequest.Destination.Tiles.Resource = streamTexture->GetInternalResource()->D3DResource();
                    dsRequest.Destination.Tiles.TiledRegionStartCoordinate = currCoord;

                    mFileCopyQueue->EnqueueRequest(&dsRequest);
                }

                {
                    // 可优化
                    std::lock_guard lock(mCopyFenceMutex);
                    mCopyFence->IncrementExpectedValue();
                    mFileCopyQueue->EnqueueSignal(mCopyFence->D3DFence(), mCopyFence->ExpectedValue());
                    uploadList->mCopyFenceValue = mCopyFence->ExpectedValue();
                    mFileCopyQueue->Submit();
                }
            }
        };

        // 将uploadList的任务提交给TaskSystem
        Tool::TaskSystem::GetInstance()->Submit(mappingAndUploadTask);
        uploadList->SetUploadListState(UploadList::State::Processing);

    }

    void DataUploader::MonitorThread() {
        while (mThreadRunning) {
            const uint32_t numTasks = mRingMonitorTaskAlloc.GetReadyToRead();
            if (numTasks == 0u) {
                WaitForSingleObject(mMonitorEvent, INFINITE);
                if (!mThreadRunning) break;
            }

            // 对每一个Allocated的UploadList进行监督
            const uint32_t startIndex = mRingMonitorTaskAlloc.GetReadIndex();
            for (uint32_t i = startIndex; i < (startIndex + numTasks); i++) {

                auto& uploadListWrap = mRingMonitorTasks[i % mRingMonitorTasks.size()];
                bool freeList{ false };

                if (uploadListWrap->mUploadState != UploadList::State::Processing) {
                    continue;
                }

                if (uploadListWrap->mCopyFenceValue <= mCopyFence->CompletedValue() &&
                    uploadListWrap->mMappingFenceValue <= mMappingFence->CompletedValue()) {
                    // Copy 与 Mapping 都完成了

                    // 通知StreamTexture更改驻留信息
                    uploadListWrap->mPendingStreamTexture->TileLoadingsCompletedCallback(uploadListWrap->mPendingLoadings);
                    
                    // 将UploadList重新放入池中
                    uploadListWrap->SetUploadListState(UploadList::State::Free);
                    uploadListWrap.Release();

                    // O(1) array compaction: move the first element to the position of the element to be freed, then reduce size by 1.
                    mRingMonitorTasks[i % mRingMonitorTasks.size()] = std::move(mRingMonitorTasks[mRingMonitorTaskAlloc.GetReadIndex()]);
                    mRingMonitorTaskAlloc.Free();
                }
            }
        }
    }

}