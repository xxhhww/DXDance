#pragma once
#include "Renderer/SoftwareVirtualTexture.h"
#include "Renderer/BuddyHeapAllocator.h"
#include "GHL/DirectStorageFactory.h"
#include "GHL/DirectStorageQueue.h"
#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"
#include "Tools/Pool.h"
#include "Tools/Ring.h"

namespace Renderer {

	class TileUploader {
	public:
		struct Task {
		public:
			enum class State {
				Free,
				Allocated,
				Processing,
				Completed
			};

		public:
			std::atomic<Task::State> mTaskState{ State::Free };

			SoftwareVirtualTexture* mPendingVirtualTexture{ nullptr };
			std::vector<D3D12_TILED_RESOURCE_COORDINATE> mPendingLoadings;
			std::vector<BuddyHeapAllocator::Allocation*> mBuddyHeapAllocations;

			uint64_t mCopyFenceValue{ 0u };		// GPU Copy    Fence Value
			uint64_t mMappingFenceValue{ 0u };	// GPU Mapping Fence Value

		public:
			inline Task() : mTaskState(State::Free) {}
			inline ~Task() = default;

			inline void SetUploadListState(Task::State state) { 
				mTaskState = state; 
			}

			inline void SetPendingStreamTexture(SoftwareVirtualTexture* pendingTexture) { 
				mPendingVirtualTexture = pendingTexture; 
			}

			inline void PushPendingLoadings(
				const D3D12_TILED_RESOURCE_COORDINATE& coord,
				BuddyHeapAllocator::Allocation* heapAllocation) {
				mPendingLoadings.push_back(coord);
				mBuddyHeapAllocations.push_back(heapAllocation);
			}

			inline void Reset() {
				mPendingVirtualTexture = nullptr;
				mPendingLoadings.clear();
				mBuddyHeapAllocations.clear();
				mMappingFenceValue = 0u;
				mCopyFenceValue = 0u;
			}

			inline bool Empty() { 
				return mPendingLoadings.empty(); 
			}

		};

		struct SlotUserDataType {
		public:
			std::optional<uint64_t> taskIndex = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserDataType>;

	public:
		TileUploader(
			const GHL::DirectStorageQueue* dStorageFileCopyQueue,
			const GHL::Fence* fileCopyFence,
			const GHL::CommandQueue* mappingQueue,
			const GHL::Fence* mappingFence
		);

		~TileUploader();

		/*
		* 分配一个Task，由ProcessThread调用
		*/
		Task* AllocateTask();

		/*
		* 将Task提交到TaskSystem中
		*/
		void SubmitTask(Task* task);

	private:
		void MonitorThread();

	private:
		// Graphics Object
		const GHL::DirectStorageQueue* mDStorageFileCopyQueue{ nullptr };
		const GHL::Fence* mFileCopyFence{ nullptr };
		const GHL::CommandQueue* mMappingQueue{ nullptr };
		const GHL::Fence* mMappingFence{ nullptr };

		// Task分配池
		Pool mTaskPoolAllocator;
		std::vector<std::unique_ptr<Task>> mTasks;

		// 监视线程处理的环形任务队列
		Tool::Ring         mRingMonitorTaskAllocator;
		std::vector<Task*> mRingMonitorTasks;

		// Thread Parameters
		bool mThreadRunning{ true };
		std::thread mMonitorThread;
		HANDLE		mMonitorEvent{ nullptr };
	};

}