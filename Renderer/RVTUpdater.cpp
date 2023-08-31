#include "Renderer/RvtUpdater.h"
#include "Renderer/RvtTiledTexture.h"
#include "Renderer/TerrainSystem.h"
#include "Renderer/RenderEngine.h"

#include "Tools/Assert.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

#include <cmath>

namespace Renderer {

	struct DrawRvtLookUpRequest {
	public:
		Math::Vector4 rect;			// Draw的目标区域(x,y为x,y --- z,w为width,height)
		int32_t mipLevel;
		Math::Vector2 tilePos;		// 索引值

		Math::Matrix4 mvpMatrix;	// 转换到图片空间中的矩阵

	public:
		inline DrawRvtLookUpRequest(const Math::Vector4& rect, int32_t mipLevel, const Math::Vector2& tilePos)
		: rect(rect)
		, mipLevel(mipLevel)
		, tilePos(tilePos) {}
	};

	RvtUpdater::RvtUpdater(TerrainSystem* terrainSystem)
	: mRenderEngine(terrainSystem->mRenderEngine)
	, mTerrainSystem(terrainSystem) 
	, mMainShaderManger(mRenderEngine->mShaderManger.get())
	, mMainResourceStateTracker(mRenderEngine->mResourceStateTracker.get())
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

		auto* copyDsQueue = mRenderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = mRenderEngine->mUploaderEngine->GetCopyFence();

		// 创建图形API对象
		{
			mRvtGrahpicsQueue = std::make_unique<GHL::GraphicsQueue>(device);
			mRvtFrameFence = std::make_unique<GHL::Fence>(device);

			mRvtFrameTracker = std::make_unique<Renderer::RingFrameTracker>(mMaxRvtFrameCount);
			mRvtResourceStateTracker = std::make_unique<Renderer::ResourceStateTracker>();
			mRvtLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mRvtFrameTracker.get());
			mRvtPoolCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mRvtFrameTracker.get());
		}

		// 创建RvtLookUpMap相关变量
		{
			// RvtLookUpMap
			TextureDesc _RvtLookUpMapDesc{};
			_RvtLookUpMapDesc.width = mTableSize;
			_RvtLookUpMapDesc.height = mTableSize;
			_RvtLookUpMapDesc.format = DXGI_FORMAT_R8G8B8A8_UNORM;
			_RvtLookUpMapDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::PixelShaderAccess;
			_RvtLookUpMapDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };

			mRvtLookUpMap = resourceAllocator->Allocate(device, _RvtLookUpMapDesc, descriptorAllocator, nullptr);
			mRvtLookUpMap->SetDebugName("RvtLookUpMap");

			renderGraph->ImportResource("RvtLookUpMap", mRvtLookUpMap);	// 主渲染线程中的Pass需要访问该资源
			resourceStateTracker->StartTracking(mRvtLookUpMap);

			// RvtDrawLookUpMapRequestBuffer
			BufferDesc _RvtDrawLookUpMapRequestBufferDesc{};
			_RvtDrawLookUpMapRequestBufferDesc.stride = sizeof(DrawRvtLookUpRequest);
			_RvtDrawLookUpMapRequestBufferDesc.size = _RvtDrawLookUpMapRequestBufferDesc.stride * mPageTable->GetCellCount();
			_RvtDrawLookUpMapRequestBufferDesc.usage = GHL::EResourceUsage::Default;
			_RvtDrawLookUpMapRequestBufferDesc.initialState = GHL::EResourceState::CopyDestination;
			_RvtDrawLookUpMapRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;

			mRvtDrawLookUpMapRequestBuffer = resourceAllocator->Allocate(device, _RvtDrawLookUpMapRequestBufferDesc, descriptorAllocator, nullptr);
			mRvtDrawLookUpMapRequestBuffer->SetDebugName("RvtDrawLookUpMapRequestBuffer");

			mRvtResourceStateTracker->StartTracking(mRvtDrawLookUpMapRequestBuffer);
		}

		// 创建Shader
		{
			mMainShaderManger->CreateGraphicsShader("",
				[](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/TerrainRenderer_ForwardPlus.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = true;
					proxy.depthStencilFormat = DXGI_FORMAT_D32_FLOAT;
					proxy.renderTargetFormatArray = {
						DXGI_FORMAT_R16G16B16A16_FLOAT,
						DXGI_FORMAT_R16G16B16A16_FLOAT,
						DXGI_FORMAT_R16G16_FLOAT,
						DXGI_FORMAT_R8G8B8A8_UNORM	// TerrainFeedback
					};
				});
		}

		// 创建QuadMesh
		{
			std::vector<Vertex> vertices;
			vertices.emplace_back(Math::Vector3{ 0, 1, 0.1f }, Math::Vector2{ 0, 1 },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{}, Math::Vector4{});
			vertices.emplace_back(Math::Vector3{ 0, 0, 0.1f }, Math::Vector2{ 0, 0 },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{}, Math::Vector4{});
			vertices.emplace_back(Math::Vector3{ 1, 0, 0.1f }, Math::Vector2{ 1, 0 },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{}, Math::Vector4{});
			vertices.emplace_back(Math::Vector3{ 1, 1, 0.1f }, Math::Vector2{ 1, 1 },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{}, Math::Vector4{});

			std::vector<uint32_t> indices;
			indices.emplace_back(0u);
			indices.emplace_back(1u);
			indices.emplace_back(2u);
			indices.emplace_back(2u);
			indices.emplace_back(3u);
			indices.emplace_back(0u);

			Renderer::BufferDesc vbDesc{};
			vbDesc.stride = sizeof(Renderer::Vertex);
			vbDesc.size = vbDesc.stride * vertices.size();
			vbDesc.usage = GHL::EResourceUsage::Default;

			Renderer::BufferDesc ibDesc{};
			ibDesc.stride = sizeof(uint32_t);
			ibDesc.size = ibDesc.stride * indices.size();
			ibDesc.usage = GHL::EResourceUsage::Default;

			quadMesh = std::make_unique<Renderer::Mesh>(
				device,
				ResourceFormat{ device, vbDesc },
				ResourceFormat{ device, ibDesc },
				nullptr,
				nullptr);

			quadMesh->LoadDataFromMemory(copyDsQueue, copyFence, vertices, indices);
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
			if (1) {	// Check Camera Position
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

				/*
				// Update TiledTexture
				{
					auto commandList = mRvtPoolCommandListAllocator->AllocateGraphicsCommandList();
					CommandBuffer commandBuffer{ commandList.Get(), mMainShaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr};
					UpdateTiledTexturePass(commandBuffer);
				}
				*/

				// Update LookUpMap
				{
					auto commandList = mRvtPoolCommandListAllocator->AllocateGraphicsCommandList();
					CommandBuffer commandBuffer{ commandList.Get(), mMainShaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
					UpdateLookUpMapPass(commandBuffer);
				}

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

	void RvtUpdater::UpdateTiledTexturePass(CommandBuffer& commandBuffer) {

	}

	void RvtUpdater::UpdateLookUpMapPass(CommandBuffer& commandBuffer) {
		// 将页表数据渲染至页表贴图
		uint64_t currentFrameNumber = mRvtFrameTracker->GetCurrFrameNumber();
		std::vector<DrawRvtLookUpRequest> drawRequests;

		for (const auto& pair : mActiveCells) {
			const auto& cell = pair.second;

			if (cell.payload.activeFrame != currentFrameNumber) {
				continue;
			}

			const auto& page = mPageTable->GetPage(cell.mipLevel);
			const auto& pageOffset = page.pageOffset;
			const auto& cellSize = page.cellSize;

			Math::Int2 lb = Math::Int2{
				cell.rect.x - pageOffset.x * cellSize,
				cell.rect.y - pageOffset.y * cellSize
			};

			while (lb.x < 0) {
				lb.x += mTableSize;
			}
			while (lb.y < 0) {
				lb.y += mTableSize;
			}

			drawRequests.emplace_back(
				Math::Vector4{ (float)lb.x, (float)lb.y, (float)cell.rect.z, (float)cell.rect.w },
				cell.mipLevel,
				Math::Vector2{
					(float)cell.payload.tileIndex.x / 255.0f,
					(float)cell.payload.tileIndex.y / 255.0f
				}
			);
		}

		if (drawRequests.empty()) {
			return;
		}

		// 对drawRequests进行排序，值更大的mipLevel排在前面。使得值更小的mipLevel在绘制时可以覆盖值更大的mipLevel
		std::sort(drawRequests.begin(), drawRequests.end(),
			[](const DrawRvtLookUpRequest& requestA, const DrawRvtLookUpRequest& requestB) {
				return requestA.mipLevel < requestB.mipLevel;
			});

		// 计算Matrix
		for (int i = 0; i < drawRequests.size(); i++) {
			// 转换到LookUpMap的ImageSpace
			float size = drawRequests[i].rect.z / mTableSize;
			drawRequests[i].mvpMatrix = Math::Matrix4{
				Math::Vector3{ drawRequests[i].rect.x / mTableSize, drawRequests[i].rect.y / mTableSize, 0.0f },
				Math::Quaternion{},
				Math::Vector3{ size, size, size }
			}.Transpose();
		}

		// 录制渲染命令
		{
			commandBuffer.PIXBeginEvent("UpdateTiledTexturePass");

			auto& rvtLookUpMapDesc = mRvtLookUpMap->GetResourceFormat().GetTextureDesc();
			uint16_t width = static_cast<uint16_t>(rvtLookUpMapDesc.width);
			uint16_t height = static_cast<uint16_t>(rvtLookUpMapDesc.height);

			// Update Pass Data
			mUpdateLookUpPassData.rvtDrawLookUpMapRequestBufferIndex = mRvtDrawLookUpMapRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateLookUpPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateLookUpPassData, sizeof(UpdateLookUpPassData));

			// 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mRvtDrawLookUpMapRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mRvtDrawLookUpMapRequestBuffer, 0u, drawRequests.data(), drawRequests.size() * sizeof(DrawRvtLookUpRequest));

			// 绘制LookUpMap
			barrierBatch = commandBuffer.TransitionImmediately(mRvtDrawLookUpMapRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += mMainResourceStateTracker->TransitionImmediately(mRvtLookUpMap, GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.ClearRenderTarget(mRvtLookUpMap);
			commandBuffer.SetRenderTarget(mRvtLookUpMap);
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState("UpdateLookUpMap");
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, quadMesh->GetVertexBuffer());
			commandBuffer.SetIndexBuffer(quadMesh->GetIndexBuffer());
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(quadMesh->GetIndexCount(), drawRequests.size(), 0u, 0u, 0u);

			commandBuffer.PIXEndEvent();

			commandBuffer.D3DCommandList()->Close();
			mRvtGrahpicsQueue->ExecuteCommandList(commandBuffer.D3DCommandList());
		}
	}

	void RvtUpdater::ActivateCell(int x, int y, int mipLevel) {
		if (mipLevel > mMaxMipLevel || mipLevel < 0 || x < 0 || y < 0 || x >= mTableSize || y >= mTableSize) {
			return;
		}

		// 找到对应的Cell
		auto& cell = mPageTable->GetCell(x, y, mipLevel);

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
		auto& cell = mPageTable->GetCell(x, y, mipLevel);
		
		// GPU已经准备渲染对应的数据了
		if (cell.payload.cellState == CellState::Loading) {
			return;
		}
		mPendingDrawTileRequests.emplace_back(x, y, mipLevel);
	}

}