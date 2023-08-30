#include "Renderer/RvtUpdater.h"
#include "Renderer/RvtTiledTexture.h"
#include "Renderer/TerrainSystem.h"
#include "Renderer/RenderEngine.h"
#include "Tools/Assert.h"
#include <cmath>

namespace Renderer {

	RvtUpdater::RvtUpdater(TerrainSystem* terrainSystem)
	: mRenderEngine(terrainSystem->mRenderEngine)
	, mTerrainSystem(terrainSystem) 
	, mRvtTiledTexture(terrainSystem->mRvtTiledTexture)
	, mTableSize(terrainSystem->tableSize)
	, mMaxMipLevel((int)std::log2(mTableSize)) {
		mFrameCompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mFrameCompletedEvent != nullptr, "Failed to Create Frame Completed Event Handle");

		mProcessThread = std::thread([this]() {
			ProcessThread();
		});

		mPageTable = new RvtPageTable(mTableSize);

		// 创建相关纹理
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		// 创建RvtLookUpMap
		{
			TextureDesc _RvtLookUpMapDesc{};
			_RvtLookUpMapDesc.width = mTableSize;
			_RvtLookUpMapDesc.height = mTableSize;
			_RvtLookUpMapDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_RvtLookUpMapDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::PixelShaderAccess;
			_RvtLookUpMapDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };

			mRvtLookUpMap = resourceAllocator->Allocate(device, _RvtLookUpMapDesc, descriptorAllocator, nullptr);
			mRvtLookUpMap->SetDebugName("RvtLookUpMap");

			renderGraph->ImportResource("RvtLookUpMap", mRvtLookUpMap);
			resourceStateTracker->StartTracking(mRvtLookUpMap);
		}

		// 创建图形API对象
		{
			mRvtGrahpicsQueue = std::make_unique<GHL::GraphicsQueue>(device);
			mRvtFrameFence = std::make_unique<GHL::Fence>(device);
			mRvtCommandListAllocator = std::make_unique<GHL::CommandAllocator>(device, D3D12_COMMAND_LIST_TYPE_DIRECT);
			mRvtCommandList = std::make_unique<GHL::CommandList>(device, mRvtCommandListAllocator->D3DCommandAllocator(), D3D12_COMMAND_LIST_TYPE_DIRECT);

			mRvtLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mRvtFrameTracker.get());
			mRvtFrameTracker = std::make_unique<Renderer::RingFrameTracker>(mMaxRvtFrameCount);
		}
	}

	RvtUpdater::~RvtUpdater() {
		mThreadRunning = false;
		SetEvent(mFrameCompletedEvent);
		mProcessThread.join();

		delete mPageTable;
	}

	void RvtUpdater::SetFrameCompletedEvent() {
		SetEvent(mFrameCompletedEvent);
	}

	void RvtUpdater::ProcessThread() {
		auto* mainFrameFence = mRenderEngine->mRenderFrameFence.get();
		uint64_t previousMainFrameFenceValue = 0u;
		bool moreTask = false;

		while (mThreadRunning) {

			uint64_t currentMainFrameFenceValue = mainFrameFence->CompletedValue();

			// Camera View Rect Changed
			if (1) {
				// TODO...
			}

			// 新的主渲染帧完成
			if (currentMainFrameFenceValue != currentMainFrameFenceValue) {
				currentMainFrameFenceValue = currentMainFrameFenceValue;

				// 对Feedback的Readback进行处理
				ProcessReadback();

				// 压入新的Rvt帧
				mRvtFrameFence->IncrementExpectedValue();
				mRvtFrameTracker->PushCurrentFrame(mRvtFrameFence->ExpectedValue());

				// 执行渲染操作
				ProcessDrawRequests();

				// 更新LookUp贴图
				UpdateLookUpMap();

				mRvtGrahpicsQueue->SignalFence(*mRvtFrameFence.get());
			}

			// 检测Frame是否过载
			if (mRvtFrameTracker->GetUsedSize() == mMaxRvtFrameCount) {
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

				UINT64 valueToWaitFor = mRvtFrameFence->ExpectedValue() - (mMaxRvtFrameCount - 1u);
				mRvtFrameFence->SetCompletionEvent(valueToWaitFor, eventHandle);

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}

			// 检测并处理渲染帧是否完成
			mRvtFrameTracker->PopCompletedFrame(mRvtFrameFence->CompletedValue());

			if (!mThreadRunning) break;
		}
	}

	void RvtUpdater::ProcessReadback() {

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
					ActivateCell(r, g, b);
				}

				// 转移到纹理的下一行，纹理的每一行除了最后一行都需要256字节对齐
				pResolvedData += (rowByteSize + 0x0ff) & ~0x0ff;
			}
			terrainReadbackBuffer->UnMap();
		}
	}

	void RvtUpdater::ProcessDrawRequests() {

	}

	void RvtUpdater::UpdateLookUpMap() {

	}

	void RvtUpdater::ActivateCell(int x, int y, int mipLevel) {
		if (mipLevel > mMaxMipLevel || mipLevel < 0 || x < 0 || y < 0 || x >= mTableSize || y >= mTableSize) {
			return;
		}

		// 找到对应的Cell
		auto cell = mPageTable->GetCell(x, y, mipLevel);

		// 检测Cell对应的贴图是否已经渲染到TileTexture中去
		if (!cell.payload.IsReady()) {
			// 贴图未渲染，则添加DrawRequest
			LoadPage(x, y, mipLevel);

			// 向上找到最近的父节点
			while (mipLevel < mMaxMipLevel && !cell.payload.IsReady()) {
				mipLevel++;
				cell = mPageTable->GetCell(x, y, mipLevel);
			}
		}

		// 激活对应的平铺贴图块
		// 如果目标Cell的渲染数据未生成，则使用其最近且有效的父节点的渲染数据
		if (cell.payload.IsReady()) {
			mRvtTiledTexture->SetActive(cell.payload.tileIndex);
			cell.payload.activeFrame = mRvtFrameTracker->GetCurrFrameNumber();
		}
	}

	void RvtUpdater::LoadPage(int x, int y, int mipLevel) {
		auto cell = mPageTable->GetCell(x, y, mipLevel);
		
		// GPU已经准备渲染对应的数据了
		if (cell.payload.cellState == CellState::Loading) {
			return;
		}
		mPendingDrawTileRequests.emplace_back(x, y, mipLevel);
	}

}