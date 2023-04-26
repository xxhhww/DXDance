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

		mThreadCompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mThreadCompletedEvent != nullptr, "Failed to Create Thread Completed Event Handle");
		mFrameCompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mFrameCompletedEvent != nullptr, "Failed to Create Frame Completed Event Handle");

		Tool::TaskSystem::GetInstance()->Submit([this]() {
			ProcessFeedbackThread();
		});
	}

	TileUpdater::~TileUpdater() {
		SetEvent(mThreadCompletedEvent);
	}

	void TileUpdater::SetFrameCompletedEvent() {
		SetEvent(mFrameCompletedEvent);
	}

	void TileUpdater::ProcessFeedbackThread() {
		while (true) {
			// 等待一个渲染帧完成
			HANDLE waitLists[] = { mThreadCompletedEvent, mFrameCompletedEvent };
			WaitForMultipleObjects(_ARRAYSIZE(waitLists), waitLists, false, INFINITE);

			// 一个渲染帧完成，为每一个StreamTexture处理ReadbackFeedback
			for (auto& pair : *mTextureStorage) {
				auto* streamTexture = pair.second.get();
				streamTexture->ProcessReadbackFeedback();
			}
		}

	}

	void TileUpdater::UpdateResidencyMipMap() {

	}

}