#include "TileUpdater.h"
#include "RingFrameTracker.h"
#include "DataUploader.h"

#include "Tools/TaskSystem.h"
#include "Tools/Assert.h"

namespace Renderer {

	TileUpdater::TileUpdater(
		const GHL::Device* device,
		RingFrameTracker* frameTracker,
		std::unordered_map<std::string, std::unique_ptr<StreamTexture>>* textureStorage,
		DataUploader* dataUploader)
	: mDevice(device) 
	, mFrameTracker(frameTracker)
	, mTextureStorage(textureStorage)
	, mDataUploader(dataUploader) {

		mFrameCompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mFrameCompletedEvent != nullptr, "Failed to Create Frame Completed Event Handle");

		mProcessFeedbackThread = std::thread([this]() {
			ProcessFeedbackThread();
		});
	}

	TileUpdater::~TileUpdater() {
		mThreadRunning = false;
		SetEvent(mFrameCompletedEvent);
		mProcessFeedbackThread.join();
		CloseHandle(mFrameCompletedEvent);
	}

	void TileUpdater::SetFrameCompletedEvent() {
		SetEvent(mFrameCompletedEvent);
	}

	void TileUpdater::ProcessFeedbackThread() {
		while (mThreadRunning) {
			// 如果此时没有任何任务，则等待一个新的渲染帧的完成
			WaitForSingleObject(mFrameCompletedEvent, INFINITE);
			if (!mThreadRunning) break;

			// 一个渲染帧完成，为每一个StreamTexture处理ReadbackFeedback
			for (auto& pair : *mTextureStorage) {
				auto* streamTexture = pair.second.get();
				streamTexture->ProcessReadbackFeedback();
				streamTexture->ProcessTileLoadings();
				streamTexture->ProcessTileEvictions();
			}
			
		}

	}

	void TileUpdater::UpdateResidencyMipMap() {

	}

}