#pragma once
#include "Renderer/StreamVirtualTexture.h"
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
		struct SlotUserDataType {
		public:
			std::optional<uint64_t> taskIndex = std::nullopt;
		};

		using Pool = Tool::Pool<SlotUserDataType>;

		struct Task {
		public:
			enum class State {
				Free,
				Allocated,
				Submitted,
			};

		public:
			Pool::Slot* mSlot = nullptr;	// ��Task��Ӧ��Pool�ϵĲ��

			std::atomic<Task::State> mTaskState{ State::Free };

			StreamVirtualTexture* mPendingStreamTexture{ nullptr };
			std::vector<D3D12_TILED_RESOURCE_COORDINATE> mPendingLoadings;
			std::vector<BuddyHeapAllocator::Allocation*> mHeapAllocations;

			uint64_t mFileCopyFenceValue{ 0u };	// GPU Copy    Fence Value
			uint64_t mMappingFenceValue{ 0u };	// GPU Mapping Fence Value(Todo Deleted)

			std::function<void()> mCompletedCallback;

		public:
			inline Task(Pool::Slot* slot) : mSlot(slot), mTaskState(State::Free) {}
			inline ~Task() = default;

			inline void SetUploadListState(Task::State state) {
				mTaskState = state;
			}

			inline void SetPendingStreamTexture(StreamVirtualTexture* pendingTexture) {
				mPendingStreamTexture = pendingTexture;
			}

			inline void PushPendingLoadings(
				const D3D12_TILED_RESOURCE_COORDINATE& coord,
				BuddyHeapAllocator::Allocation* heapAllocation) {
				mPendingLoadings.push_back(coord);
				mHeapAllocations.push_back(heapAllocation);
			}

			inline void Reset() {
				mPendingStreamTexture = nullptr;
				mPendingLoadings.clear();
				mHeapAllocations.clear();
				mMappingFenceValue = 0u;
				mFileCopyFenceValue = 0u;
			}

			inline bool Empty() {
				return mPendingLoadings.empty();
			}

			inline const auto& GetTaskState() const {
				return mTaskState;
			}

		};

	public:
		TileUploader(
			GHL::DirectStorageQueue* dStorageFileCopyQueue,
			GHL::Fence* fileCopyFence,
			GHL::CommandQueue* mappingQueue,
			GHL::Fence* mappingFence
		);

		~TileUploader();

		/*
		* ����һ��Task����ProcessThread����
		*/
		Task* AllocateTask();

		/*
		* ��Task�ύ��TaskSystem�У��������ѹ������
		*/
		void SubmitTask(Task* task);

		/*
		* �ύGPU����
		*/
		void SignalGPUTask();

	private:
		void MonitorThread();

	private:
		// Graphics Object
		GHL::DirectStorageQueue* mDStorageFileCopyQueue{ nullptr };
		GHL::Fence* mFileCopyFence{ nullptr };
		GHL::CommandQueue* mMappingQueue{ nullptr };
		GHL::Fence* mMappingFence{ nullptr };

		// Task�����
		Pool mPoolTaskAllocator;
		std::vector<std::unique_ptr<Task>> mTasks;

		// �����̴߳���Ļ����������
		Tool::Ring         mRingMonitorTaskAllocator;
		std::vector<Task*> mRingMonitorTasks;

		// Thread Parameters
		bool mThreadRunning{ true };
		std::thread mMonitorThread;
		HANDLE		mMonitorEvent{ nullptr };
	};

}