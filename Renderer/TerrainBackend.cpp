#include "Renderer/TerrainBackend.h"
#include "Renderer/TerrainRenderer.h"

namespace Renderer {

	TerrainBackend::TerrainBackend(TerrainRenderer* renderer)
	: mRenderer(renderer) {}

	TerrainBackend::~TerrainBackend() {

	}

	// 初始化
	void TerrainBackend::Initialize() {
		auto* renderEngine = mRenderer->GetRenderEngine();

		mDStorageFileQueue = mRenderer->GetDStorageFileQueue();
		mCopyFence = mRenderer->GetCopyFence();


	}

	// 后台线程
	void TerrainBackend::BackendThread() {

	}

	TerrainBackend::EvictionDelay::EvictionDelay(uint32_t nFrames) {
		mEvictionsBuffer.resize(nFrames);
	}

	TerrainBackend::EvictionDelay::~EvictionDelay() {
	}

	void TerrainBackend::EvictionDelay::MoveToNextFrame() {
		// start with A, B, C
		// after swaps, have C, A, B
		// then insert C, A, BC
		// then clear 0, A, BC
		uint32_t lastMapping = (uint32_t)mEvictionsBuffer.size() - 1;
		for (uint32_t i = lastMapping; i > 0; i--)
		{
			mEvictionsBuffer[i].swap(mEvictionsBuffer[i - 1]);
		}
		mEvictionsBuffer.back().insert(mEvictionsBuffer.back().end(), mEvictionsBuffer[0].begin(), mEvictionsBuffer[0].end());
		mEvictionsBuffer[0].clear();
	}

	void TerrainBackend::EvictionDelay::Clear() {
		for (auto& buffer : mEvictionsBuffer) {
			buffer.clear();
		}
	}

	void TerrainBackend::EvictionDelay::Rescue(std::vector<TerrainNodeID>& nodeIDs) {

	}

}