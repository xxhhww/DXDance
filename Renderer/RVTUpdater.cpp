#include "Renderer/RVTUpdater.h"
#include "Renderer/TerrainSystem.h"
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

			// 新的渲染帧完成，对Feedback的Readback进行处理
			ProcessReadback();
		}
	}

	void RVTUpdater::ProcessReadback() {

		auto& queuedReadbacks = mTerrainSystem->mQueuedReadbacks;
		auto& terrainReadbackBuffers = mTerrainSystem->mTerrainReadbackBuffers;

		// 寻找最新鲜的Feedback
		bool feedbackFound = false;
		uint64_t latestFeedbackFenceValue = 0;
		uint32_t targetFeedbackIndex{ 0u };
		for (size_t i = 0; i < queuedReadbacks.size(); i++) {
			if (queuedReadbacks.at(i).isFresh) {
				uint64_t feedbackFenceValue = queuedReadbacks.at(i).renderFrameFenceValue;
				if ((!feedbackFound) || (latestFeedbackFenceValue <= feedbackFenceValue)) {
					feedbackFound = true;
					targetFeedbackIndex = i;
					latestFeedbackFenceValue = feedbackFenceValue;
					// this feedback will either be used or skipped. either way it is "consumed"
					queuedReadbacks.at(i).isFresh = false;
				}
			}
		}

		if (!feedbackFound) {
			return;
		}

		{
			TextureWrap& terrainFeedbackMap = mTerrainSystem->mTerrainFeedbackMap;
			auto& textureDesc = terrainFeedbackMap->GetResourceFormat().GetTextureDesc();

			BufferWrap& terrainReadbackBuffer = terrainReadbackBuffers.at(targetFeedbackIndex);
			uint8_t* pResolvedData = terrainReadbackBuffer->Map();

			uint32_t height = textureDesc.height;
			uint32_t width = textureDesc.width;
			uint32_t rowByteSize = width * GHL::GetFormatStride(textureDesc.format);

			for (uint32_t y = 0u; y < height; y++) {
				for (uint32_t x = 0u; x < rowByteSize; x++) {
					uint8_t r = pResolvedData[x++];
					uint8_t g = pResolvedData[x++];
					uint8_t b = pResolvedData[x++];
					uint8_t a = pResolvedData[x++];

					// 激活页表

				}

				// 转移到纹理的下一行，纹理的每一行除了最后一行都需要256字节对齐
				pResolvedData += (rowByteSize + 0x0ff) & ~0x0ff;
			}
			terrainReadbackBuffer->UnMap();

			/*
			uint32_t height = GetNumTilesHeight();
			uint32_t width = GetNumTilesWidth();

			uint8_t* pTileRow = mMinMipMapCache.data();
			for (uint32_t y = 0; y < height; y++) {
				for (uint32_t x = 0; x < width; x++) {
					// clamp to the maximum we are tracking (not tracking packed mips)
					uint8_t desired = std::min(pResolvedData[x], mNumStandardMips);
					desired = 2u;
					uint8_t initialValue = pTileRow[x];
					SetMinMip(initialValue, x, y, desired);
					pTileRow[x] = desired;
				} // end loop over x
				pTileRow += width;
				pResolvedData += (width + 0x0ff) & ~0x0ff;

			} // end loop over y

			terrainReadbackBuffer->UnMap();
			*/
			// 更新pendingEvictions
			// mPendingTileEvictions.Rescue(*mTileMappingState);
		}
	}

}