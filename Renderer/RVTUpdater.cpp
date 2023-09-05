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

	public:
		inline GPUDrawRvtTiledMapRequest(const Math::Vector4& wsTileRect, const Math::Vector4& isTileRect, const Math::Matrix4& mvp)
		: tileRectInWorldSpace(wsTileRect)
		, tileRectInImageSpace(isTileRect)
		, mvpMatrix(mvp) {}
	};

	struct GPUDrawRvtLookUpMapRequest {
	public:
		Math::Vector4 rect;			// Draw的目标区域(x,y为x,y --- z,w为width,height)
		int32_t mipLevel;
		float pad1;
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
	, mMaxRvtFrameCount(3u)
	, mTableSize(256u)
	, mMaxMipLevel((int)std::log2(mTableSize))
	, mRvtRadius(1024.0f)
	, mCellSize(2 * mRvtRadius / mTableSize)
	, mChangeViewDis((1.0f / 8.0f) * 2 * mRvtRadius)
	, mLimitPerFrame(2u) {

		// 杂项初始化
		{
			mFrameCompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			ASSERT_FORMAT(mFrameCompletedEvent != nullptr, "Failed to Create Frame Completed Event Handle");

			mRvtTiledTexture = std::make_unique<RvtTiledTexture>(mRenderEngine);
			mPageTable = std::make_unique<RvtPageTable>(mTableSize);
			mLoadingDrawTileRequests.resize(mMaxRvtFrameCount);
			mPendingDrawTileRequests.resize(mMaxRvtFrameCount);
		}

		// 创建相关纹理
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		auto* copyDsQueue = mRenderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = mRenderEngine->mUploaderEngine->GetCopyFence();

		// 创建图形API对象并设置回调函数
		{
			mRvtGrahpicsQueue = std::make_unique<GHL::GraphicsQueue>(device);
			mRvtGrahpicsQueue->SetDebugName("RvtUpdater");
			mRvtFrameFence = std::make_unique<GHL::Fence>(device);

			mRvtFrameTracker = std::make_unique<Renderer::RingFrameTracker>(mMaxRvtFrameCount);
			mRvtResourceStateTracker = std::make_unique<Renderer::ResourceStateTracker>();
			mRvtLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mRvtFrameTracker.get());
			mRvtPoolCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mRvtFrameTracker.get());
		
			mRvtFrameTracker->AddFrameCompletedCallBack([this](const size_t& frameIndex) {
				this->OnRvtFrameCompleted(frameIndex);
			});
		}

		// 创建RvtLookUpMap相关变量
		{
			// RvtLookUpMap
			TextureDesc _RvtLookUpMapDesc{};
			_RvtLookUpMapDesc.width = mTableSize;
			_RvtLookUpMapDesc.height = mTableSize;
			_RvtLookUpMapDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
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
			_RvtDrawTiledMapRequestsBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
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
			_RvtDrawLookUpMapRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
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
						DXGI_FORMAT_R16G16B16A16_FLOAT	// RvtLookUpMap
					};
				});

			mMainShaderManger->CreateGraphicsShader("UpdateRvtTiledMap",
				[](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/UpdateRvtTiledMap.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						DXGI_FORMAT_R16G16B16A16_FLOAT,	// TiledMapAlbedo
						DXGI_FORMAT_R16G16B16A16_FLOAT	// TiledMapNormal
					};
				});
		}

		// 创建QuadMesh
		{
			std::vector<Vertex> vertices;
			vertices.emplace_back(Math::Vector3{ 0.0f, 1.0f, 0.0f }, Math::Vector2{ 0.0f, 1.0f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{}, Math::Vector4{});
			vertices.emplace_back(Math::Vector3{ 0.0f, 0.0f, 0.0f }, Math::Vector2{ 0.0f, 0.0f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{}, Math::Vector4{});
			vertices.emplace_back(Math::Vector3{ 1.0f, 0.0f, 0.0f }, Math::Vector2{ 1.0f, 0.0f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{}, Math::Vector4{});
			vertices.emplace_back(Math::Vector3{ 1.0f, 1.0f, 0.0f }, Math::Vector2{ 1.0f, 1.0f },
				Math::Vector3{}, Math::Vector3{}, Math::Vector3{}, Math::Vector4{});

			std::vector<uint32_t> indices;
			indices.emplace_back(0u);
			indices.emplace_back(2u);
			indices.emplace_back(1u);
			indices.emplace_back(2u);
			indices.emplace_back(0u);
			indices.emplace_back(3u);

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
				// 新的Rvt帧开始
				for (auto it = mActiveCells.begin(); it != mActiveCells.end(); ++it) {
					const auto cellIndex = it->second;
					auto& cell = mPageTable->GetCell(cellIndex.x, cellIndex.y, cellIndex.mipLevel);
					cell.activeUsed = false;
				}

				// 压入新的Rvt帧
				mRvtFrameFence->IncrementExpectedValue();
				mRvtFrameTracker->PushCurrentFrame(mRvtFrameFence->ExpectedValue());
				mRvtFrameTracker->SetCurrentFrameUserData(std::underlying_type<RvtFrameType>::type(RvtFrameType::ProcessFeedbackFrame));

				// 对Feedback的Readback进行处理
				ProcessReadback(currentMainFrameFenceValue);
				
				// Update RvtLookUpMap
				UpdateRvtLookUpMapPass();

				// Update TiledTexture
				UpdateTiledTexturePass();

				mRvtGrahpicsQueue->SignalFence(*mRvtFrameFence.get());
			}

			// 检测RvtFrame是否过载
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

			if (!mThreadRunning) { 
				break;
			}
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
			}
		);

		// 每次只处理限制个数的DrawTileRequest
		int32_t count = mLimitPerFrame;
		std::vector<GPUDrawRvtTiledMapRequest> gpuDrawRequests;
		while (count > 0 && drawTileRequests.size() > 0u) {
			// 取出mip值最大的DrawTileRequest
			DrawTileRequest drawTileRequest = drawTileRequests.back();
			drawTileRequests.pop_back();
			// DrawTileRequest drawTileRequest = drawTileRequests.at(drawTileRequests.size() - count);

			// 从TiledMap中请求一个可用的tilePos
			const Math::Int2 tilePos = mRvtTiledTexture->RequestTile();
			mRvtTiledTexture->SetActive(tilePos);
			drawTileRequest.tilePos = tilePos;

			// 如果需要，将申请到的tilePos的对应的Cell进行修改，并删除对应关系
			auto it = mActiveCells.find(tilePos);
			if (it != mActiveCells.end()) {
				const auto cellIndex = it->second;
				auto& retiredCell = mPageTable->GetCell(cellIndex.x, cellIndex.y, cellIndex.mipLevel);

				retiredCell.cellState = CellState::InActive;
				retiredCell.tilePos = RvtPageLevelTableCell::smInvalidTilePos;
				mActiveCells.erase(it);
			}

			// 计算tilePos对应的图像空间下的tileRect
			const auto& tileSizeWithPadding = mRvtTiledTexture->GetTileSizeWithPadding();
			const Math::Int4 tileRectInImageSpace = Math::Int4{
				tilePos.x * (int32_t)tileSizeWithPadding,
				tilePos.y * (int32_t)tileSizeWithPadding,
				(int32_t)tileSizeWithPadding,
				(int32_t)tileSizeWithPadding
			};

			// 网格规格化
			int x = drawTileRequest.x;
			int y = drawTileRequest.y;
			int perSize = (int)std::pow(2, drawTileRequest.mipLevel);
			x = x - x % perSize;
			y = y - y % perSize;

			uint32_t paddingEffect = mRvtTiledTexture->GetPaddingSize() * perSize * (mCurrRvtRect.z / mTableSize) / mRvtTiledTexture->GetTileSize();
			Math::Vector4 tileRectInWorldSpace = Math::Vector4{
				mCurrRvtRect.x + ((float)x / mTableSize) * mCurrRvtRect.z - paddingEffect,
				mCurrRvtRect.y + ((float)y / mTableSize) * mCurrRvtRect.w - paddingEffect,
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
			mvpMatrix._11 = 0.0f;
			mvpMatrix._22 = 0.0f;
			mvpMatrix._33 = 0.0f;
			mvpMatrix._44 = 0.0f;
			mvpMatrix._11 = r - l;
			mvpMatrix._14 = l;
			mvpMatrix._22 = t - b;
			mvpMatrix._24 = b;
			mvpMatrix._34 = 1.0f;
			mvpMatrix._44 = 1.0f;

			gpuDrawRequests.emplace_back(tileRectInWorldSpaceOverlapped, tileRectInImageSpaceOverlapped, mvpMatrix);
			mLoadingDrawTileRequests.at(mRvtFrameTracker->GetCurrFrameIndex()).emplace_back(drawTileRequest);

			count--;
		}

		// 将剩下未处理的drawTileRequests放到下一帧去
		if (!drawTileRequests.empty()) {
			size_t currFrameIndex = mRvtFrameTracker->GetCurrFrameIndex();
			size_t nextFrameIndex = (currFrameIndex + 1u) % mRvtFrameTracker->GetMaxSize();

			auto& currDrawTileRequests = mPendingDrawTileRequests.at(currFrameIndex);
			auto& nextDrawTileRequests = mPendingDrawTileRequests.at(nextFrameIndex);

			nextDrawTileRequests.insert(nextDrawTileRequests.end(), currDrawTileRequests.begin(), currDrawTileRequests.end());
			currDrawTileRequests.clear();
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
			mUpdateRvtTiledMapPassData.drawRequestBufferIndex = mRvtDrawTiledMapRequestsBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.terrainHeightMapIndex = mTerrainSystem->heightMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.terrainNormalMapIndex = mTerrainSystem->normalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.terrainSplatMapIndex = mTerrainSystem->splatMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.terrainMeterSize = mTerrainSystem->worldMeterSize;
			mUpdateRvtTiledMapPassData.terrainHeightScale = mTerrainSystem->worldHeightScale;

			mUpdateRvtTiledMapPassData.rChannelAlbedoMapIndex = mTerrainSystem->grassAlbedoMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.rChannelNormalMapIndex = mTerrainSystem->grassNormalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.rChannelHeightMapIndex = mTerrainSystem->grassHeightMap->GetSRDescriptor()->GetHeapIndex();
			// mUpdateRvtTiledMapPassData.rChannelRoughnessMapIndex = mTerrainSystem->grassRoughnessMap->GetSRDescriptor()->GetHeapIndex();

			mUpdateRvtTiledMapPassData.gChannelAlbedoMapIndex = mTerrainSystem->mudAlbedoMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.gChannelNormalMapIndex = mTerrainSystem->mudNormalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.gChannelHeightMapIndex = mTerrainSystem->mudHeightMap->GetSRDescriptor()->GetHeapIndex();
			// mUpdateRvtTiledMapPassData.gChannelRoughnessMapIndex = mTerrainSystem->mudRoughnessMap->GetSRDescriptor()->GetHeapIndex();

			mUpdateRvtTiledMapPassData.bChannelAlbedoMapIndex = mTerrainSystem->cliffAlbedoMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.bChannelNormalMapIndex = mTerrainSystem->cliffNormalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.bChannelHeightMapIndex = mTerrainSystem->cliffHeightMap->GetSRDescriptor()->GetHeapIndex();
			// mUpdateRvtTiledMapPassData.bChannelRoughnessMapIndex = mTerrainSystem->cliffRoughnessMap->GetSRDescriptor()->GetHeapIndex();

			mUpdateRvtTiledMapPassData.aChannelAlbedoMapIndex = mTerrainSystem->snowAlbedoMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.aChannelNormalMapIndex = mTerrainSystem->snowNormalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdateRvtTiledMapPassData.aChannelHeightMapIndex = mTerrainSystem->snowHeightMap->GetSRDescriptor()->GetHeapIndex();
			// mUpdateRvtTiledMapPassData.aChannelRoughnessMapIndex = mTerrainSystem->snowRoughness->GetSRDescriptor()->GetHeapIndex();

			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRvtTiledMapPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRvtTiledMapPassData, sizeof(UpdateRvtTiledMapPassData));

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

			commandBuffer.SetRenderTargets({ tileMaps.at(0), tileMaps.at(1) });
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

		uint32_t testCount = 2u;
		for (const auto& pair : mActiveCells) {
			const auto  cellIndex = pair.second;
			const auto& cell = mPageTable->GetCell(cellIndex.x, cellIndex.y, cellIndex.mipLevel);

			if (!cell.activeUsed) {
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

			if (testCount > 0) {
				testCount--;
				continue;
			}

			gpuDrawRequests.emplace_back(
				Math::Vector4{ (float)lb.x, (float)lb.y, (float)cell.rect.z, (float)cell.rect.w },
				cell.mipLevel,
				Math::Vector2{
					(float)cell.tilePos.x,
					(float)cell.tilePos.y
				}
			);
		}

		if (gpuDrawRequests.empty()) {
			return;
		}

		// 对drawRequests进行排序，值更大的mipLevel排在前面。使得值更小的mipLevel在绘制时可以覆盖值更大的mipLevel
		std::sort(gpuDrawRequests.begin(), gpuDrawRequests.end(),
			[](const GPUDrawRvtLookUpMapRequest& requestA, const GPUDrawRvtLookUpMapRequest& requestB) {
				return requestA.mipLevel > requestB.mipLevel;
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
			commandBuffer.ClearRenderTarget(mRvtLookUpMap);
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
		auto cell = mPageTable->GetCell(x, y, mipLevel);

		// 检测Cell对应的贴图是否已经渲染到TileTexture中去
		if (cell.cellState == CellState::InActive) {
			// 贴图未渲染，则添加DrawRequest
			LoadPage(x, y, mipLevel);

			// 向上找到最近的且被激活的父节点
			while (mipLevel < mMaxMipLevel && (cell.cellState != CellState::Active)) {
				mipLevel++;
				cell = mPageTable->GetCell(x, y, mipLevel);
			}
		}

		// 激活对应的平铺贴图块
		// 如果目标Cell的渲染数据未生成，则使用其最近且有效的父节点的渲染数据
		if (cell.cellState == CellState::Active) {
			mRvtTiledTexture->SetActive(cell.tilePos);
			mPageTable->GetCell(x, y, mipLevel).activeUsed = true;
		}
	}

	void RvtUpdater::LoadPage(int x, int y, int mipLevel) {
		auto& cell = mPageTable->GetCell(x, y, mipLevel);

		if (cell.cellState == CellState::Loading || cell.cellState == CellState::Active) {
			return;
		}

		cell.cellState = CellState::Loading;
		mPendingDrawTileRequests.at(mRvtFrameTracker->GetCurrFrameIndex()).emplace_back(x, y, mipLevel);
	}

	void RvtUpdater::OnRvtFrameCompleted(uint8_t frameIndex) {
		// 完成的帧所处理的DrawTileRequests
		auto& completedDrawTileRequests = mLoadingDrawTileRequests.at(frameIndex);

		if (completedDrawTileRequests.empty()) {
			return;
		}

		// 更新ActiveCells
		for (const auto& drawTileRequest : completedDrawTileRequests) {
			mActiveCells.insert(std::pair<Math::Int2, CellIndex>(
				drawTileRequest.tilePos,
				CellIndex{ drawTileRequest.x, drawTileRequest.y, drawTileRequest.mipLevel })
			);

			// 修改目标Cell的数据
			auto& targetCell = mPageTable->GetCell(drawTileRequest.x, drawTileRequest.y, drawTileRequest.mipLevel);
			targetCell.activeUsed = true;
			targetCell.cellState = CellState::Active;
			targetCell.tilePos = drawTileRequest.tilePos;
		}

		completedDrawTileRequests.clear();
	}

}