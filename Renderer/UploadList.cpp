#include "UploadList.h"

namespace Renderer {

	UploadList::UploadList()
	: mUploadState(State::Free) {}

	void UploadList::SetUpdateTileMapTask(const UpdateTileMapTask& task) {
		mUpdateTileMapTask = task;
	}

	void UploadList::SetDataUploadTask(const DataUploadTask& task) {
		mDataUploadTask = task;
	}

	void UploadList::AddCompletedCallBack(const CompletedCallBack& cb) {
		mCompletedCallBacks.push_back(cb);
	}

	void UploadList::Clear() {
		mUpdateTileMapTask = nullptr;
		mDataUploadTask = nullptr;
		mCompletedCallBacks.clear();
	}

	void UploadList::SetUploadState(const UploadList::State& state) {
		mUploadState = state;
	}

	void UploadList::SetExpectedFenceValue(uint64_t fenceValue) {
		mExpectedFenceValue = fenceValue;
	}

	void UploadList::ExecuteCompletedCallBacks() {
		for (const auto& cb : mCompletedCallBacks) {
			cb();
		}
	}

}