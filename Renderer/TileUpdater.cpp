#include "TileUpdater.h"
#include "RingFrameTracker.h"
#include "DataUploader.h"

#include "GHL/Fence.h"

#include "Tools/TaskSystem.h"
#include "Tools/Assert.h"

namespace Renderer {

	TileUpdater::TileUpdater(
		const GHL::Device* device,
		GHL::Fence* renderFrameFence,
		RingFrameTracker* frameTracker,
		std::unordered_map<std::string, std::unique_ptr<StreamTexture>>* textureStorage,
		DataUploader* dataUploader)
	: mDevice(device) 
	, mRenderFrameFence(renderFrameFence)
	, mFrameTracker(frameTracker)
	, mTextureStorage(textureStorage)
	, mDataUploader(dataUploader) {
		mProcessFeedbackThread = std::thread([this]() {
			ProcessFeedbackThread();
		});
	}

	TileUpdater::~TileUpdater() {
		mThreadRunning = false;
		mProcessFeedbackThread.join();
	}

	void TileUpdater::ProcessFeedbackThread() {
		bool moreTasks = false;
		uint64_t prevFrameFenceValue = 0u;
		while (mThreadRunning) {
			// 如果此时没有任何任务，则等待一个新的渲染帧的完成
			uint64_t completedFrameFenceVaule = mRenderFrameFence->CompletedValue();
			if (completedFrameFenceVaule != prevFrameFenceValue) {
				for (auto& pair : *mTextureStorage) {
					auto* streamTexture = pair.second.get();
					streamTexture->ProcessReadbackFeedback();
					if (streamTexture->IsStale()) {
						moreTasks = true;
					}
				}
			}

			if (moreTasks) {
				for (auto& pair : *mTextureStorage) {
					auto* streamTexture = pair.second.get();
					if (streamTexture->IsStale()) {
						continue;
					}
					streamTexture->ProcessTileLoadings();
					streamTexture->ProcessTileEvictions();
				}
			}

		}

	}

	void TileUpdater::UpdateResidencyMipMap() {

	}

}