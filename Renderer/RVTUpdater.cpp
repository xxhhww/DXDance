#include "Renderer/RvtUpdater.h"
#include "Renderer/RvtTiledTexture.h"
#include "Renderer/TerrainSystem.h"
#include "Renderer/RenderEngine.h"

#include "Tools/Assert.h"
#include "Math/Vector.h"
#include "Math/Matrix.h"

#include <cmath>

namespace Renderer {

	struct GPUDrawRvtTiledMapRequest {
	public:
		Math::Vector4 tileRectInWorldSpace;
		Math::Vector4 tileRectInImageSpace;
		Math::Matrix4 mvpMatrix;

		inline GPUDrawRvtTiledMapRequest(const Math::Vector4& wsTileRect, const Math::Vector4& isTileRect, const Math::Matrix4& mvp)
		: tileRectInWorldSpace(wsTileRect)
		, tileRectInImageSpace(isTileRect)
		, mvpMatrix(mvp) {}
	};

	struct GPUDrawRvtLookUpMapRequest {
	public:
		Math::Vector4 rect;			// Draw的目标区域(x,y为x,y --- z,w为width,height)
		int32_t mipLevel;
		Math::Vector2 tilePos;		// 索引值

		Math::Matrix4 mvpMatrix;	// 转换到图片空间中的矩阵

	public:
		inline GPUDrawRvtLookUpMapRequest(const Math::Vector4& rect, int32_t mipLevel, const Math::Vector2& tilePos)
		: rect(rect)
		, mipLevel(mipLevel)
		, tilePos(tilePos) {}
	};

	RvtUpdater::RvtUpdater(TerrainSystem* terrainSystem)
	: mRenderEngine(terrainSystem->mRenderEngine)
	, mTerrainSystem(terrainSystem) 
	, mMainShaderManger(mRenderEngine->mShaderManger.get())
	, mMainResourceStateTracker(mRenderEngine->mResourceStateTracker.get())
	, mMainDescriptorAllocator(mRenderEngine->mDescriptorAllocator.get())
	, mRvtTiledTexture(terrainSystem->mRvtTiledTexture)
	, mMaxRvtFrameCount(3u)
	, mTableSize(256u)
	, mMaxMipLevel((int)std::log2(mTableSize))
	, mRvtRadius(1024.0f)
	, mCellSize(2 * mRvtRadius / mTableSize)
	, mChangeViewDis((1.0f / 8.0f) * 2 * mRvtRadius)
	, mLimitPerFrame(2u) {
		mFrameCompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mFrameCompletedEvent != nullptr, "Failed to Create Frame Completed Event Handle");

		mPageTable = new RvtPageTable(mTableSize);
		mPendingDrawTileRequests.resize(mMaxRvtFrameCount);

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
			mRvtGrahpicsQueue->SetDebugName("RvtUpdater");
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

			// RvtDrawTiledMapRequestsBuffer
			BufferDesc _RvtDrawTiledMapRequestsBufferDesc{};
			_RvtDrawTiledMapRequestsBufferDesc.stride = sizeof(GPUDrawRvtTiledMapRequest);
			_RvtDrawTiledMapRequestsBufferDesc.size = _RvtDrawTiledMapRequestsBufferDesc.stride * mRvtTiledTexture->GetAllTileCount();
			_RvtDrawTiledMapRequestsBufferDesc.usage = GHL::EResourceUsage::Default;
			_RvtDrawTiledMapRequestsBufferDesc.initialState = GHL::EResourceState::Common;
			_RvtDrawTiledMapRequestsBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mRvtDrawTiledMapRequestsBuffer = resourceAllocator->Allocate(device, _RvtDrawTiledMapRequestsBufferDesc, descriptorAllocator, nullptr);
			mRvtDrawTiledMapRequestsBuffer->SetDebugName("RvtDrawTiledMapRequestsBuffer");

			mRvtResourceStateTracker->StartTracking(mRvtDrawTiledMapRequestsBuffer);

			// RvtDrawLookUpMapRequestBuffer
			BufferDesc _RvtDrawLookUpMapRequestBufferDesc{};
			_RvtDrawLookUpMapRequestBufferDesc.stride = sizeof(GPUDrawRvtLookUpMapRequest);
			_RvtDrawLookUpMapRequestBufferDesc.size = _RvtDrawLookUpMapRequestBufferDesc.stride * mPageTable->GetCellCount();
			_RvtDrawLookUpMapRequestBufferDesc.usage = GHL::EResourceUsage::Default;
			_RvtDrawLookUpMapRequestBufferDesc.initialState = GHL::EResourceState::Common;
			_RvtDrawLookUpMapRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;

			mRvtDrawLookUpMapRequestBuffer = resourceAllocator->Allocate(device, _RvtDrawLookUpMapRequestBufferDesc, descriptorAllocator, nullptr);
			mRvtDrawLookUpMapRequestBuffer->SetDebugName("RvtDrawLookUpMapRequestBuffer");

			mRvtResourceStateTracker->StartTracking(mRvtDrawLookUpMapRequestBuffer);
		}

		// 创建Shader
		{
			mMainShaderManger->CreateGraphicsShader("UpdateRvtLookUpMap",
				[](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/UpdateRvtLookUpMap.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						DXGI_FORMAT_R8G8B8A8_UNORM		// RvtLookUpMap
					};
				});

			mMainShaderManger->CreateGraphicsShader("UpdateRvtTiledMap",
				[](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/UpdateRvtTiledMap.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						DXGI_FORMAT_R16G16B16A16_FLOAT	// TiledMap
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

		// 启动处理线程
		mProcessThread = std::thread([this]() {
			this->ProcessThread();
		});
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

			Math::Int2 fixedCenter = GetFixedCenter(GetFixedPos(
				mRenderEngine->mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.position));
			mCurrRvtRect = Math::Vector4{ fixedCenter.x - mRvtRadius, fixedCenter.y - mRvtRadius, 2 * mRvtRadius, 2 * mRvtRadius };

			// Camera View Rect Changed
			if (1) {	// Check Camera Position
				// TODO...
			}

			// 新的主渲染帧完成
			if (previousMainFrameFenceValue != currentMainFrameFenceValue) {
				previousMainFrameFenceValue = currentMainFrameFenceValue;

				// 压入新的Rvt帧
				mRvtFrameFence->IncrementExpectedValue();
				mRvtFrameTracker->PushCurrentFrame(mRvtFrameFence->ExpectedValue());

				// 对Feedback的Readback进行处理
				ProcessReadback(currentMainFrameFenceValue);
				
				// Update RvtLookUpMap
				UpdateRvtLookUpMapPass();

				// Update TiledTexture
				UpdateTiledTexturePass();

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

	void RvtUpdater::ProcessReadback(uint64_t completedFenceValue) {

		auto& queuedReadbacks = mTerrainSystem->mQueuedReadbacks;
		auto& terrainReadbackBuffers = mTerrainSystem->mTerrainReadbackBuffers;

		// 寻找最新鲜的Feedback
		bool feedbackFound = false;
		uint64_t latestFeedbackFenceValue = 0;
		uint32_t targetFeedbackIndex{ 0u };
		for (size_t i = 0; i < queuedReadbacks.size(); i++) {
			if (queuedReadbacks.at(i).isFresh) {
				uint64_t feedbackFenceValue = queuedReadbacks.at(i).renderFrameFenceValue;
				if ((completedFenceValue >= feedbackFenceValue) &&
					((!feedbackFound) || (latestFeedbackFenceValue <= feedbackFenceValue))) {
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
			uint32_t rowByteSize = (width * GHL::GetFormatStride(textureDesc.format) + 0x0ff) & ~0x0ff;

			for (uint32_t y = 0u; y < height; y++) {
				for (uint32_t x = 0u; x < rowByteSize;) {
					uint8_t r = pResolvedData[x++];
					uint8_t g = pResolvedData[x++];
					uint8_t b = pResolvedData[x++];
					uint8_t a = pResolvedData[x++];

					if (a == 0u) {
						continue;
					}
					
					// 激活页表
					ActivateCell(r, g, b);
				}

				// 转移到纹理的下一行，纹理的每一行除了最后一行都需要256字节对齐
				pResolvedData += rowByteSize;
			}
			terrainReadbackBuffer->UnMap();
		}

		return;
	}

	void RvtUpdater::UpdateTiledTexturePass() {
		auto& drawTileRequests = mPendingDrawTileRequests.at(mRvtFrameTracker->GetCurrFrameIndex());

		if (drawTileRequests.empty()) {
			return;
		}

		// 按照mipLevel进行排序
		std::sort(drawTileRequests.begin(), drawTileRequests.end(), 
			[](const DrawTileRequest& requestA, const DrawTileRequest& requestB) {
				return requestA.mipLevel < requestB.mipLevel;
		});

		// 每次只处理限制个数的DrawTileRequest
		int32_t count = mLimitPerFrame;
		std::vector<GPUDrawRvtTiledMapRequest> gpuDrawRequests;
		while (count > 0 && drawTileRequests.size() > 0u) {
			// 取出mip值最大的DrawTileRequest
			const DrawTileRequest request = drawTileRequests.back();
			drawTileRequests.pop_back();

			const RvtPageTableNodeCell cell = mPageTable->GetCell(request.x, request.y, request.mipLevel);
			if (cell.payload.cellState != CellState::InActive) {
				continue;
			}

			// 从TiledMap中请求一个可用的tile
			const Math::Int2 tilePos = mRvtTiledTexture->RequestTile();
			mRvtTiledTexture->SetActive(tilePos);

			// 计算tilePos对应的图像空间下的tileRect
			const auto& tileSizeWithPadding = mRvtTiledTexture->GetTileSizeWithPadding();
			const Math::Int4 tileRectInImageSpace = Math::Int4{
				tilePos.x * tileSizeWithPadding,
				tilePos.y * tileSizeWithPadding,
				tileSizeWithPadding,
				tileSizeWithPadding
			};

			// 网格规格化
			int x = request.x;
			int y = request.y;
			int perSize = (int)std::pow(2, request.mipLevel);
			x = x - x % perSize;
			y = y - y % perSize;

			uint32_t paddingEffect = mRvtTiledTexture->GetPaddingSize() * perSize * (mCurrRvtRect.z / mTableSize) / mRvtTiledTexture->GetTileSize();
			Math::Vector4 tileRectInWorldSpace = Math::Vector4{
				mCurrRvtRect.x + ((float)x / mTableSize) * mCurrRvtRect.z - paddingEffect,
				mCurrRvtRect.y + ((float)x / mTableSize) * mCurrRvtRect.w - paddingEffect,
				mCurrRvtRect.z * ((float)perSize / mTableSize) + 2.0f * paddingEffect,
				mCurrRvtRect.w * ((float)perSize / mTableSize) + 2.0f * paddingEffect
			};

			// 地块矩形
			const Math::Vector2 terrainMeterSize = mTerrainSystem->worldMeterSize;
			Math::Vector4 terrainRectInWorldSpace = Math::Vector4{
				- (terrainMeterSize.x / 2.0f),
				- (terrainMeterSize.y / 2.0f),
				terrainMeterSize.x,
				terrainMeterSize.y
			};

			// 将tileRectInWorldSpace与terrainRectInWorldSpace相重叠
			float tileRectInWorldSpaceOverlappedXMin = std::max(tileRectInWorldSpace.x, terrainRectInWorldSpace.x);
			float tileRectInWorldSpaceOverlappedZMin = std::max(tileRectInWorldSpace.y, terrainRectInWorldSpace.y);
			float tileRectInWorldSpaceOverlappedXMax = std::min(tileRectInWorldSpace.x + tileRectInWorldSpace.z, terrainRectInWorldSpace.x + terrainRectInWorldSpace.z);
			float tileRectInWorldSpaceOverlappedZMax = std::min(tileRectInWorldSpace.y + tileRectInWorldSpace.w, terrainRectInWorldSpace.y + terrainRectInWorldSpace.w);
			Math::Vector4 tileRectInWorldSpaceOverlapped = Math::Vector4{
				tileRectInWorldSpaceOverlappedXMin,
				tileRectInWorldSpaceOverlappedZMin,
				tileRectInWorldSpaceOverlappedXMax - tileRectInWorldSpaceOverlappedXMin,
				tileRectInWorldSpaceOverlappedZMax - tileRectInWorldSpaceOverlappedZMin
			};

			// 计算tileRect在WorldSpace与ImageSpace之间的缩放关系
			float scaleFactorWidth  = tileRectInImageSpace.z / tileRectInWorldSpace.z;
			float scaleFactorHeight = tileRectInImageSpace.w / tileRectInWorldSpace.w;

			Math::Vector4 tileRectInImageSpaceOverlapped = Math::Vector4{
				tileRectInImageSpace.x + (tileRectInWorldSpaceOverlapped.x - tileRectInWorldSpace.x) * scaleFactorWidth,
				tileRectInImageSpace.y + (tileRectInWorldSpaceOverlapped.y - tileRectInWorldSpace.y) * scaleFactorHeight,
				tileRectInWorldSpaceOverlapped.z * scaleFactorWidth,
				tileRectInWorldSpaceOverlapped.w * scaleFactorHeight
			};

			// 构建从局部空间到图像空间的MVP矩阵
			const Math::Int2 tiledMapSize = mRvtTiledTexture->GetTiledMapSize();

			// 左右下上
			float l = tileRectInImageSpaceOverlapped.x * 2.0f / tiledMapSize.x - 1.0f;
			float r = (tileRectInImageSpaceOverlapped.x + tileRectInImageSpaceOverlapped.z) * 2.0f / tiledMapSize.x - 1.0f;
			float b = tileRectInImageSpaceOverlapped.y * 2.0f / tiledMapSize.y - 1;
			float t = (tileRectInImageSpaceOverlapped.y + tileRectInImageSpaceOverlapped.w) * 2.0f / tiledMapSize.y - 1.0f;
			
			// 构建
			Math::Matrix4 mvpMatrix{};
			mvpMatrix.m[0][0] = r - l;
			mvpMatrix.m[0][3] = l;
			mvpMatrix.m[1][1] = t - b;
			mvpMatrix.m[1][3] = b;
			mvpMatrix.m[2][3] = -1.0f;
			mvpMatrix.m[3][3] = 1.0f;

			gpuDrawRequests.emplace_back(tileRectInWorldSpaceOverlapped, tileRectInImageSpaceOverlapped, mvpMatrix.Transpose());

			count--;
		}

		if (gpuDrawRequests.empty()) {
			return;
		}

		// 录制渲染命令
		{
			auto commandList = mRvtPoolCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = mMainDescriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), mMainShaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateTiledMapPass");

			auto& tileMaps = mRvtTiledTexture->GetTiledMaps();
			ASSERT_FORMAT(tileMaps.size() > 0u, "TileMaps Is Empty!");
			auto& rvtTileMapDesc = tileMaps.at(0)->GetResourceFormat().GetTextureDesc();
			uint16_t width = static_cast<uint16_t>(rvtTileMapDesc.width);
			uint16_t height = static_cast<uint16_t>(rvtTileMapDesc.height);

			// Update Pass Data
			...修改PassData;
			mUpdateRvtTiledMapPassData.drawRequestBufferIndex = mRvtDrawTiledMapRequestsBuffer->GetSRDescriptor()->GetHeapIndex();
			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRvtTiledTexturePassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRvtTiledMapPassData, sizeof(UpdateRvtTiledTexturePassData));

			// 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mRvtDrawTiledMapRequestsBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mRvtDrawTiledMapRequestsBuffer, 0u, gpuDrawRequests.data(), gpuDrawRequests.size() * sizeof(GPUDrawRvtTiledMapRequest));

			// 绘制LookUpMap
			barrierBatch = commandBuffer.TransitionImmediately(mRvtDrawTiledMapRequestsBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += mMainResourceStateTracker->TransitionImmediately(tileMaps.at(0), GHL::EResourceState::RenderTarget);
			barrierBatch += mMainResourceStateTracker->TransitionImmediately(tileMaps.at(1), GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetRenderTargets({ tileMaps.at(0), tileMaps.at(1) }, nullptr);
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState("UpdateRvtTiledMap");
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, quadMesh->GetVertexBuffer());
			commandBuffer.SetIndexBuffer(quadMesh->GetIndexBuffer());
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(quadMesh->GetIndexCount(), gpuDrawRequests.size(), 0u, 0u, 0u);

			commandBuffer.PIXEndEvent();

			commandList->Close();
			mRvtGrahpicsQueue->ExecuteCommandList(commandList->D3DCommandList());
		}

	}

	void RvtUpdater::UpdateRvtLookUpMapPass() {
		// 将页表数据渲染至页表贴图
		uint64_t currentFrameNumber = mRvtFrameTracker->GetCurrFrameNumber();
		std::vector<GPUDrawRvtLookUpMapRequest> gpuDrawRequests;

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

			gpuDrawRequests.emplace_back(
				Math::Vector4{ (float)lb.x, (float)lb.y, (float)cell.rect.z, (float)cell.rect.w },
				cell.mipLevel,
				Math::Vector2{
					(float)cell.payload.tileIndex.x / 255.0f,
					(float)cell.payload.tileIndex.y / 255.0f
				}
			);
		}

		if (gpuDrawRequests.empty()) {
			return;
		}

		// 对drawRequests进行排序，值更大的mipLevel排在前面。使得值更小的mipLevel在绘制时可以覆盖值更大的mipLevel
		std::sort(gpuDrawRequests.begin(), gpuDrawRequests.end(),
			[](const GPUDrawRvtLookUpMapRequest& requestA, const GPUDrawRvtLookUpMapRequest& requestB) {
				return requestA.mipLevel < requestB.mipLevel;
			});

		// 计算Matrix
		for (int i = 0; i < gpuDrawRequests.size(); i++) {
			// 转换到LookUpMap的ImageSpace
			float size = gpuDrawRequests[i].rect.z / mTableSize;
			gpuDrawRequests[i].mvpMatrix = Math::Matrix4{
				Math::Vector3{ gpuDrawRequests[i].rect.x / mTableSize, gpuDrawRequests[i].rect.y / mTableSize, 0.0f },
				Math::Quaternion{},
				Math::Vector3{ size, size, size }
			}.Transpose();
		}

		// 录制渲染命令
		{
			auto commandList = mRvtPoolCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = mMainDescriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), mMainShaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateRvtLookUpMapPass");

			auto& rvtLookUpMapDesc = mRvtLookUpMap->GetResourceFormat().GetTextureDesc();
			uint16_t width = static_cast<uint16_t>(rvtLookUpMapDesc.width);
			uint16_t height = static_cast<uint16_t>(rvtLookUpMapDesc.height);

			// Update Pass Data
			mUpdateRvtLookUpMapPassData.drawRequestBufferIndex = mRvtDrawLookUpMapRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRvtLookUpMapPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRvtLookUpMapPassData, sizeof(UpdateRvtLookUpMapPassData));

			// 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mRvtDrawLookUpMapRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mRvtDrawLookUpMapRequestBuffer, 0u, gpuDrawRequests.data(), gpuDrawRequests.size() * sizeof(GPUDrawRvtLookUpMapRequest));

			// 绘制LookUpMap
			barrierBatch = commandBuffer.TransitionImmediately(mRvtDrawLookUpMapRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += mMainResourceStateTracker->TransitionImmediately(mRvtLookUpMap, GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetRenderTarget(mRvtLookUpMap);
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState("UpdateRvtLookUpMap");
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, quadMesh->GetVertexBuffer());
			commandBuffer.SetIndexBuffer(quadMesh->GetIndexBuffer());
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(quadMesh->GetIndexCount(), gpuDrawRequests.size(), 0u, 0u, 0u);

			commandBuffer.PIXEndEvent();

			commandList->Close();
			mRvtGrahpicsQueue->ExecuteCommandList(commandList->D3DCommandList());
		}
	}

	void RvtUpdater::ActivateCell(int x, int y, int mipLevel) {
		if (mipLevel > mMaxMipLevel || mipLevel < 0 || x < 0 || y < 0 || x >= mTableSize || y >= mTableSize) {
			return;
		}

		// 找到对应的Cell(拷贝一份)
		RvtPageTableNodeCell cell = mPageTable->GetCell(x, y, mipLevel);

		// 检测Cell对应的贴图是否已经渲染到TileTexture中去
		if (cell.payload.cellState == CellState::InActive) {
			// 贴图未渲染，则添加DrawRequest
			LoadPage(x, y, mipLevel);

			// 向上找到最近的父节点
			while (mipLevel < mMaxMipLevel && (cell.payload.cellState != CellState::Active)) {
				mipLevel++;
				cell = mPageTable->GetCell(x, y, mipLevel);
			}
		}

		// 激活对应的平铺贴图块
		// 如果目标Cell的渲染数据未生成，则使用其最近且有效的父节点的渲染数据
		if (cell.payload.cellState == CellState::Active) {
			mRvtTiledTexture->SetActive(cell.payload.tileIndex);
			mPageTable->GetCell(x, y, mipLevel).payload.activeFrame = mRvtFrameTracker->GetCurrFrameNumber();
		}
	}

	void RvtUpdater::LoadPage(int x, int y, int mipLevel) {
		auto& cell = mPageTable->GetCell(x, y, mipLevel);

		if (cell.payload.cellState == CellState::Loading) {
			return;
		}
		cell.payload.cellState = CellState::Loading;

		mPendingDrawTileRequests.at(mRvtFrameTracker->GetCurrFrameIndex()).emplace_back(x, y, mipLevel);
	}

}