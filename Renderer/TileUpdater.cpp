#include "TileUpdater.h"
#include "RingFrameTracker.h"

namespace Renderer {

	TileUpdater::TileUpdater(const GHL::Device* device, RingFrameTracker* frameTracker)
	: mDevice(device) 
	, mFrameTracker(frameTracker) {
		mFrameTracker->AddFrameCompletedCallBack([this](const size_t& frameIndex) {
			FrameCompletedCallback(frameIndex);
		});
	}

	void TileUpdater::FrameCompletedCallback(uint8_t frameIndex) {
		// 每一个渲染帧完成后，通知ProcessFeedback线程，对Feedback进行处理
	}

}