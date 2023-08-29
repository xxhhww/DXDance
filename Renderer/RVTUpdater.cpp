#include "Renderer/RVTUpdater.h"
#include "Tools/Assert.h"

namespace Renderer {

	RVTUpdater::RVTUpdater(TerrainSystem* terrainSystem)
	: mTerrainSystem(terrainSystem) {
		mFrameCompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mFrameCompletedEvent != nullptr, "Failed to Create Frame Completed Event Handle");

		mProcessFeedbackThread = std::thread([this]() {
			ProcessFeedbackThread();
		});
	}

	RVTUpdater::~RVTUpdater() {
		mThreadRunning = false;
		SetEvent(mFrameCompletedEvent);
		mProcessFeedbackThread.join();
	}

	void RVTUpdater::SetFrameCompletedEvent() {
		SetEvent(mFrameCompletedEvent);
	}

	void RVTUpdater::ProcessFeedbackThread() {
		while (mThreadRunning) {
			WaitForSingleObject(mFrameCompletedEvent, INFINITE);
			if (!mThreadRunning) break;

			// 新的渲染帧完成，对Feedback进行处理

		}
	}

}