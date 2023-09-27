#include "Renderer/TileUploader.h"

namespace Renderer {

	TileUploader::TileUploader(
		GHL::DirectStorageQueue* dStorageFileCopyQueue,
		GHL::Fence* fileCopyFence,
		GHL::CommandQueue* mappingQueue,
		GHL::Fence* mappingFence
	) 
	: mDStorageFileCopyQueue(dStorageFileCopyQueue)
	, mFileCopyFence(fileCopyFence)
	, mMappingQueue(mappingQueue)
	, mMappingFence(mappingFence) 
	, mRingMonitorTasks(512)
	, mRingMonitorTaskAllocator(512) {

		// for first loop
		mFileCopyFence->IncrementExpectedValue();
		// 启动监视线程
		mMonitorEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mMonitorEvent != nullptr, "Failed to Create Monitor Event Handle");
		mMonitorThread = std::thread([this]() {
			MonitorThread();
		});
	}

	TileUploader::~TileUploader() {
		mThreadRunning = false;
		SetEvent(mMonitorEvent);
		mMonitorThread.join();
	}

	TileUploader::Task* TileUploader::AllocateTask() {
		Pool::Slot* slot = mPoolTaskAllocator.Allocate();

		if (!slot->userData.taskIndex) {
			mTasks.emplace_back(std::make_unique<Task>(slot));
			slot->userData.taskIndex = mTasks.size() - 1u;
		}

		Task* task = mTasks.at(*slot->userData.taskIndex).get();
		task->Reset();
		task->SetUploadListState(TileUploader::Task::State::Allocated);

		// 通知监视线程，开始监视UploadList
		mRingMonitorTasks[mRingMonitorTaskAllocator.GetWriteIndex()] = task;
		mRingMonitorTaskAllocator.Allocate();
		SetEvent(mMonitorEvent);

		return mTasks.at(*slot->userData.taskIndex).get();
	}

	void TileUploader::SubmitTask(Task* task) {
		// 在MappingQueue(CopyQueue)中压入显存映射操作
		auto* streamTexture = task->mPendingStreamTexture;

		uint32_t numCoords = task->mPendingLoadings.size();
		for (uint32_t i = 0u; i < numCoords; i++) {
			const auto& currCoord = task->mPendingLoadings.at(i);
			const auto* currAlloc = task->mHeapAllocations.at(i);

			D3D12_TILE_REGION_SIZE resourceRegionSizes{ 1u, FALSE, 0, 0, 0 };
			std::vector<D3D12_TILE_RANGE_FLAGS> rangeFlags(1u, D3D12_TILE_RANGE_FLAG_NONE);

			mMappingQueue->D3DCommandQueue()->UpdateTileMappings(
				streamTexture->D3DResource(),
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

		// 在从文件中读取数据资源至显存
		{
			auto* streamTexture = task->mPendingStreamTexture;
			const auto& fileFormat = streamTexture->GetFileFormat();
			const auto* fileHandle = streamTexture->GetFileHandle();

			DSTORAGE_REQUEST dsRequest{};
			dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
			dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TILES;
			dsRequest.Options.CompressionFormat = (DSTORAGE_COMPRESSION_FORMAT)fileFormat.GetCompressionFormat();
			dsRequest.Destination.Tiles.TileRegionSize = D3D12_TILE_REGION_SIZE{ 1, FALSE, 0, 0, 0 };
			dsRequest.Source.File.Source = fileHandle->GetDStorageFile();
			dsRequest.UncompressedSize = D3D12_TILED_RESOURCE_TILE_SIZE_IN_BYTES;

			uint32_t numCoords = task->mPendingLoadings.size();
			for (uint32_t i = 0u; i < numCoords; i++) {
				const auto& currCoord = task->mPendingLoadings.at(i);
				auto fileOffset = fileFormat.GetFileOffset(currCoord);

				dsRequest.Source.File.Offset = fileOffset.offset;
				dsRequest.Source.File.Size = fileOffset.numBytes;

				dsRequest.Destination.Tiles.Resource = streamTexture->D3DResource();
				dsRequest.Destination.Tiles.TiledRegionStartCoordinate = currCoord;

				mDStorageFileCopyQueue->EnqueueRequest(&dsRequest);
			}
		}

		task->mFileCopyFenceValue = mFileCopyFence->ExpectedValue();
		task->SetUploadListState(TileUploader::Task::State::Submitted);
	}

	void TileUploader::SignalGPUTask() {
		mDStorageFileCopyQueue->EnqueueSignal(*mFileCopyFence);
		mDStorageFileCopyQueue->Submit();

		// for next loop
		mFileCopyFence->IncrementExpectedValue();
	}

	void TileUploader::MonitorThread() {
		// 监视线程，对Task的GPU任务进行监视
		while (mThreadRunning) {
			uint32_t numAllocatedTasks = mPoolTaskAllocator.AllocatedSize();
			if (numAllocatedTasks == 0u) {
				WaitForSingleObject(mMonitorEvent, INFINITE);
				if (!mThreadRunning) break;
			}

			const uint32_t numTasks = mRingMonitorTaskAllocator.GetReadyToRead();
			const uint32_t startIndex = mRingMonitorTaskAllocator.GetReadIndex();
			for (uint32_t i = startIndex; i < (startIndex + numTasks); i++) {

				auto* task = mRingMonitorTasks[i % mRingMonitorTasks.size()];
				if (task->GetTaskState() != Task::State::Submitted) {
					continue;
				}

				if (task->mFileCopyFenceValue <= mFileCopyFence->CompletedValue()) {
					// 显存映射与数据上传的任务均被完成了

					// 通知StreamVirtualTexture更改驻留信息，并通知UpdateResidencyThread更新MinMipMap
					task->mPendingStreamTexture->OnCopyCompleted(task->mPendingLoadings);

					// 将task重新放入池中
					task->SetUploadListState(Task::State::Free);
					mPoolTaskAllocator.Deallocate(task->mSlot);

					// O(1) array compaction: move the first element to the position of the element to be freed, then reduce size by 1.
					mRingMonitorTasks[i % mRingMonitorTasks.size()] = std::move(mRingMonitorTasks[mRingMonitorTaskAllocator.GetReadIndex()]);
					mRingMonitorTaskAllocator.Free();
				}
			}
		}
	}

}