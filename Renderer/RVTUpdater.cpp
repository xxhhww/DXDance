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
		Math::Vector4 rect;			// Draw��Ŀ������(x,yΪx,y --- z,wΪwidth,height)
		int32_t mipLevel;
		float pad1;
		Math::Vector2 tilePos;		// ����ֵ

		Math::Matrix4 mvpMatrix;	// ת����ͼƬ�ռ��еľ���

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

		// �����ʼ��
		{
			mFrameCompletedEvent = CreateEvent(nullptr, FALSE, FALSE, nullptr);
			ASSERT_FORMAT(mFrameCompletedEvent != nullptr, "Failed to Create Frame Completed Event Handle");

			mRvtTiledTexture = std::make_unique<RvtTiledTexture>(mRenderEngine);
			mPageTable = std::make_unique<RvtPageTable>(mTableSize);
			mLoadingDrawTileRequests.resize(mMaxRvtFrameCount);
			mPendingDrawTileRequests.resize(mMaxRvtFrameCount);
		}

		// �����������
		auto* device = mRenderEngine->mDevice.get();
		auto* renderGraph = mRenderEngine->mRenderGraph.get();
		auto* resourceAllocator = mRenderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = mRenderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = mRenderEngine->mResourceStateTracker.get();

		auto* copyDsQueue = mRenderEngine->mUploaderEngine->GetMemoryCopyQueue();
		auto* copyFence = mRenderEngine->mUploaderEngine->GetCopyFence();

		// ����ͼ��API�������ûص�����
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

		// ����RvtLookUpMap��ر���
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

			renderGraph->ImportResource("RvtLookUpMap", mRvtLookUpMap);	// ����Ⱦ�߳��е�Pass��Ҫ���ʸ���Դ
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

		// ����Shader
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

		// ����QuadMesh
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

		// ���������߳�
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

			// �µ�����Ⱦ֡���
			if (previousMainFrameFenceValue != currentMainFrameFenceValue) {
				previousMainFrameFenceValue = currentMainFrameFenceValue;
				// �µ�Rvt֡��ʼ
				for (auto it = mActiveCells.begin(); it != mActiveCells.end(); ++it) {
					const auto cellIndex = it->second;
					auto& cell = mPageTable->GetCell(cellIndex.x, cellIndex.y, cellIndex.mipLevel);
					cell.activeUsed = false;
				}

				// ѹ���µ�Rvt֡
				mRvtFrameFence->IncrementExpectedValue();
				mRvtFrameTracker->PushCurrentFrame(mRvtFrameFence->ExpectedValue());
				mRvtFrameTracker->SetCurrentFrameUserData(std::underlying_type<RvtFrameType>::type(RvtFrameType::ProcessFeedbackFrame));

				// ��Feedback��Readback���д���
				ProcessReadback(currentMainFrameFenceValue);
				
				// Update RvtLookUpMap
				UpdateRvtLookUpMapPass();

				// Update TiledTexture
				UpdateTiledTexturePass();

				mRvtGrahpicsQueue->SignalFence(*mRvtFrameFence.get());
			}

			// ���RvtFrame�Ƿ����
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

			if (!mThreadRunning) { 
				break;
			}
		}
	}

	void RvtUpdater::ProcessReadback(uint64_t completedFenceValue) {

		auto& queuedReadbacks = mTerrainSystem->mQueuedReadbacks;
		auto& terrainReadbackBuffers = mTerrainSystem->mTerrainReadbackBuffers;

		// Ѱ�������ʵ�Feedback
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
					
					// ����ҳ��
					ActivateCell(r, g, b);
				}

				// ת�Ƶ��������һ�У������ÿһ�г������һ�ж���Ҫ256�ֽڶ���
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

		// ����mipLevel��������
		std::sort(drawTileRequests.begin(), drawTileRequests.end(), 
			[](const DrawTileRequest& requestA, const DrawTileRequest& requestB) {
				return requestA.mipLevel < requestB.mipLevel;
			}
		);

		// ÿ��ֻ�������Ƹ�����DrawTileRequest
		int32_t count = mLimitPerFrame;
		std::vector<GPUDrawRvtTiledMapRequest> gpuDrawRequests;
		while (count > 0 && drawTileRequests.size() > 0u) {
			// ȡ��mipֵ����DrawTileRequest
			DrawTileRequest drawTileRequest = drawTileRequests.back();
			drawTileRequests.pop_back();
			// DrawTileRequest drawTileRequest = drawTileRequests.at(drawTileRequests.size() - count);

			// ��TiledMap������һ�����õ�tilePos
			const Math::Int2 tilePos = mRvtTiledTexture->RequestTile();
			mRvtTiledTexture->SetActive(tilePos);
			drawTileRequest.tilePos = tilePos;

			// �����Ҫ�������뵽��tilePos�Ķ�Ӧ��Cell�����޸ģ���ɾ����Ӧ��ϵ
			auto it = mActiveCells.find(tilePos);
			if (it != mActiveCells.end()) {
				const auto cellIndex = it->second;
				auto& retiredCell = mPageTable->GetCell(cellIndex.x, cellIndex.y, cellIndex.mipLevel);

				retiredCell.cellState = CellState::InActive;
				retiredCell.tilePos = RvtPageLevelTableCell::smInvalidTilePos;
				mActiveCells.erase(it);
			}

			// ����tilePos��Ӧ��ͼ��ռ��µ�tileRect
			const auto& tileSizeWithPadding = mRvtTiledTexture->GetTileSizeWithPadding();
			const Math::Int4 tileRectInImageSpace = Math::Int4{
				tilePos.x * (int32_t)tileSizeWithPadding,
				tilePos.y * (int32_t)tileSizeWithPadding,
				(int32_t)tileSizeWithPadding,
				(int32_t)tileSizeWithPadding
			};

			// ������
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

			// �ؿ����
			const Math::Vector2 terrainMeterSize = mTerrainSystem->worldMeterSize;
			Math::Vector4 terrainRectInWorldSpace = Math::Vector4{
				- (terrainMeterSize.x / 2.0f),
				- (terrainMeterSize.y / 2.0f),
				terrainMeterSize.x,
				terrainMeterSize.y
			};

			// ��tileRectInWorldSpace��terrainRectInWorldSpace���ص�
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

			// ����tileRect��WorldSpace��ImageSpace֮������Ź�ϵ
			float scaleFactorWidth  = tileRectInImageSpace.z / tileRectInWorldSpace.z;
			float scaleFactorHeight = tileRectInImageSpace.w / tileRectInWorldSpace.w;

			Math::Vector4 tileRectInImageSpaceOverlapped = Math::Vector4{
				tileRectInImageSpace.x + (tileRectInWorldSpaceOverlapped.x - tileRectInWorldSpace.x) * scaleFactorWidth,
				tileRectInImageSpace.y + (tileRectInWorldSpaceOverlapped.y - tileRectInWorldSpace.y) * scaleFactorHeight,
				tileRectInWorldSpaceOverlapped.z * scaleFactorWidth,
				tileRectInWorldSpaceOverlapped.w * scaleFactorHeight
			};

			// �����Ӿֲ��ռ䵽ͼ��ռ��MVP����
			const Math::Int2 tiledMapSize = mRvtTiledTexture->GetTiledMapSize();

			// ��������
			float l = tileRectInImageSpaceOverlapped.x * 2.0f / tiledMapSize.x - 1.0f;
			float r = (tileRectInImageSpaceOverlapped.x + tileRectInImageSpaceOverlapped.z) * 2.0f / tiledMapSize.x - 1.0f;
			float b = tileRectInImageSpaceOverlapped.y * 2.0f / tiledMapSize.y - 1;
			float t = (tileRectInImageSpaceOverlapped.y + tileRectInImageSpaceOverlapped.w) * 2.0f / tiledMapSize.y - 1.0f;
			
			// ����
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

		// ��ʣ��δ�����drawTileRequests�ŵ���һ֡ȥ
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

		// ¼����Ⱦ����
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

			// �����������ϴ���GPU��ȥ
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mRvtDrawTiledMapRequestsBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mRvtDrawTiledMapRequestsBuffer, 0u, gpuDrawRequests.data(), gpuDrawRequests.size() * sizeof(GPUDrawRvtTiledMapRequest));

			// ����LookUpMap
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
		// ��ҳ��������Ⱦ��ҳ����ͼ
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

		// ��drawRequests��������ֵ�����mipLevel����ǰ�档ʹ��ֵ��С��mipLevel�ڻ���ʱ���Ը���ֵ�����mipLevel
		std::sort(gpuDrawRequests.begin(), gpuDrawRequests.end(),
			[](const GPUDrawRvtLookUpMapRequest& requestA, const GPUDrawRvtLookUpMapRequest& requestB) {
				return requestA.mipLevel > requestB.mipLevel;
			});

		// ����Matrix
		for (int i = 0; i < gpuDrawRequests.size(); i++) {
			// ת����LookUpMap��ImageSpace
			float size = gpuDrawRequests[i].rect.z / mTableSize;
			gpuDrawRequests[i].mvpMatrix = Math::Matrix4{
				Math::Vector3{ gpuDrawRequests[i].rect.x / mTableSize, gpuDrawRequests[i].rect.y / mTableSize, 0.0f },
				Math::Quaternion{},
				Math::Vector3{ size, size, size }
			}.Transpose();
		}

		// ¼����Ⱦ����
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

			// �����������ϴ���GPU��ȥ
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mRvtDrawLookUpMapRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mRvtDrawLookUpMapRequestBuffer, 0u, gpuDrawRequests.data(), gpuDrawRequests.size() * sizeof(GPUDrawRvtLookUpMapRequest));

			// ����LookUpMap
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

		// �ҵ���Ӧ��Cell(����һ��)
		auto cell = mPageTable->GetCell(x, y, mipLevel);

		// ���Cell��Ӧ����ͼ�Ƿ��Ѿ���Ⱦ��TileTexture��ȥ
		if (cell.cellState == CellState::InActive) {
			// ��ͼδ��Ⱦ�������DrawRequest
			LoadPage(x, y, mipLevel);

			// �����ҵ�������ұ�����ĸ��ڵ�
			while (mipLevel < mMaxMipLevel && (cell.cellState != CellState::Active)) {
				mipLevel++;
				cell = mPageTable->GetCell(x, y, mipLevel);
			}
		}

		// �����Ӧ��ƽ����ͼ��
		// ���Ŀ��Cell����Ⱦ����δ���ɣ���ʹ�����������Ч�ĸ��ڵ����Ⱦ����
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
		// ��ɵ�֡�������DrawTileRequests
		auto& completedDrawTileRequests = mLoadingDrawTileRequests.at(frameIndex);

		if (completedDrawTileRequests.empty()) {
			return;
		}

		// ����ActiveCells
		for (const auto& drawTileRequest : completedDrawTileRequests) {
			mActiveCells.insert(std::pair<Math::Int2, CellIndex>(
				drawTileRequest.tilePos,
				CellIndex{ drawTileRequest.x, drawTileRequest.y, drawTileRequest.mipLevel })
			);

			// �޸�Ŀ��Cell������
			auto& targetCell = mPageTable->GetCell(drawTileRequest.x, drawTileRequest.y, drawTileRequest.mipLevel);
			targetCell.activeUsed = true;
			targetCell.cellState = CellState::Active;
			targetCell.tilePos = drawTileRequest.tilePos;
		}

		completedDrawTileRequests.clear();
	}

}