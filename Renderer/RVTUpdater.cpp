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

		// �����������
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		// ����RvtLookUpMap
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

		// ����ͼ��API����
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

			// �µ�����Ⱦ֡���
			if (currentMainFrameFenceValue != currentMainFrameFenceValue) {
				currentMainFrameFenceValue = currentMainFrameFenceValue;

				// ��Feedback��Readback���д���
				ProcessReadback();

				// ѹ���µ�Rvt֡
				mRvtFrameFence->IncrementExpectedValue();
				mRvtFrameTracker->PushCurrentFrame(mRvtFrameFence->ExpectedValue());

				// ִ����Ⱦ����
				ProcessDrawRequests();

				// ����LookUp��ͼ
				UpdateLookUpMap();

				mRvtGrahpicsQueue->SignalFence(*mRvtFrameFence.get());
			}

			// ���Frame�Ƿ����
			if (mRvtFrameTracker->GetUsedSize() == mMaxRvtFrameCount) {
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

				UINT64 valueToWaitFor = mRvtFrameFence->ExpectedValue() - (mMaxRvtFrameCount - 1u);
				mRvtFrameFence->SetCompletionEvent(valueToWaitFor, eventHandle);

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}

			// ��Ⲣ������Ⱦ֡�Ƿ����
			mRvtFrameTracker->PopCompletedFrame(mRvtFrameFence->CompletedValue());

			if (!mThreadRunning) break;
		}
	}

	void RvtUpdater::ProcessReadback() {

		auto& queuedReadbacks = mTerrainSystem->mQueuedReadbacks;
		auto& terrainReadbackBuffers = mTerrainSystem->mTerrainReadbackBuffers;

		// Ѱ�������ʵ�Feedback
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

					// ����ҳ��
					ActivateCell(r, g, b);
				}

				// ת�Ƶ��������һ�У������ÿһ�г������һ�ж���Ҫ256�ֽڶ���
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

		// �ҵ���Ӧ��Cell
		auto cell = mPageTable->GetCell(x, y, mipLevel);

		// ���Cell��Ӧ����ͼ�Ƿ��Ѿ���Ⱦ��TileTexture��ȥ
		if (!cell.payload.IsReady()) {
			// ��ͼδ��Ⱦ�������DrawRequest
			LoadPage(x, y, mipLevel);

			// �����ҵ�����ĸ��ڵ�
			while (mipLevel < mMaxMipLevel && !cell.payload.IsReady()) {
				mipLevel++;
				cell = mPageTable->GetCell(x, y, mipLevel);
			}
		}

		// �����Ӧ��ƽ����ͼ��
		// ���Ŀ��Cell����Ⱦ����δ���ɣ���ʹ�����������Ч�ĸ��ڵ����Ⱦ����
		if (cell.payload.IsReady()) {
			mRvtTiledTexture->SetActive(cell.payload.tileIndex);
			cell.payload.activeFrame = mRvtFrameTracker->GetCurrFrameNumber();
		}
	}

	void RvtUpdater::LoadPage(int x, int y, int mipLevel) {
		auto cell = mPageTable->GetCell(x, y, mipLevel);
		
		// GPU�Ѿ�׼����Ⱦ��Ӧ��������
		if (cell.payload.cellState == CellState::Loading) {
			return;
		}
		mPendingDrawTileRequests.emplace_back(x, y, mipLevel);
	}

}