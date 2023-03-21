#pragma once
#include "GHL/pbh.h"
#include <functional>
#include <atomic>

namespace Renderer {

	class Uploader;

	class UploadList {
		friend class Uploader;
	public:
		using UpdateTileMapTask = std::function<void(ID3D12CommandQueue*)>;        // ����TileMap������
		using DataUploadTask    = std::function<void(ID3D12GraphicsCommandList*)>; // �����ϴ�������
		using CompletedCallBack = std::function<void()>;                           // ������ɵĻص�

		enum class State {
			Free,
			Allocated,
			Submitted,
			Processing,
			Completed
		};

	public:
		UploadList();
		~UploadList() = default;
		
		void SetUpdateTileMapTask(const UpdateTileMapTask& task);
		
		void SetDataUploadTask(const DataUploadTask& task);

		void AddCompletedCallBack(const CompletedCallBack& cb);

		void Clear();

		void SetUploadState(const State& state);

		void SetExpectedFenceValue(uint64_t fenceValue);

		inline const auto IsDataUploadTask() const { return mDataUploadTask != nullptr; }

		inline const auto GetExpectedFenceValue() const { return mExpectedFenceValue; }

		void ExecuteCompletedCallBacks();

	private:
		std::atomic<UploadList::State> mUploadState{ State::Free };
		uint64_t mExpectedFenceValue{ 0u };

		UpdateTileMapTask mUpdateTileMapTask{ nullptr };
		DataUploadTask mDataUploadTask{ nullptr };

		std::vector<CompletedCallBack> mCompletedCallBacks;

	};

}