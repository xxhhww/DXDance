#include "Renderer/RuntimeVirtualTextureBackend.h"
#include "Renderer/RuntimeVirtualTextureAtlas.h"
#include "Renderer/RuntimeVirtualTextureAtlasTileCache.h"
#include "Renderer/TerrainTextureArray.h"
#include "Renderer/RenderEngine.h"

#include "Tools/Assert.h"

namespace Renderer {

	struct GpuUpdateRuntimeVirtualTextureAtlasRequest {
	public:
		Math::Matrix4 mvpMatrix;
		Math::Vector4 tileOffset;
		Math::Vector4 blendOffset;
	};

	struct GpuUpdateLookupPageTableRequest {
	public:
		uint32_t tilePosX;
		uint32_t tilePosY;
		int32_t  pageLevel;
		float    pad1;

		Math::Matrix4 mvpMatrix;	// 转换到图片空间中的矩阵
	};

	RuntimeVirtualTextureBackend::RuntimeVirtualTextureBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting)
	: mRenderer(renderer)
	, mTerrainSetting(terrainSetting) 
	, mRvtPageTables(mRenderer->mRvtLookupPageTables) {

		CreateGraphicsObject();

		// 启动线程
		mThread = std::thread([this]() {
				this->BackendThread();
			}
		);
	}

	RuntimeVirtualTextureBackend::~RuntimeVirtualTextureBackend() {
		mThreadRunning = false;
	}

	void RuntimeVirtualTextureBackend::Preload() {

	}

	void RuntimeVirtualTextureBackend::BackendThread() {
		return;

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* mainRenderFrameFence = renderEngine->mRenderFrameFence.get();
		uint64_t previousMainFrameFenceValue = 0u;

		while (mThreadRunning) {
			uint64_t currentMainFrameFenceValue = mainRenderFrameFence->CompletedValue();

			// 新的主渲染帧完成
			if (previousMainFrameFenceValue != currentMainFrameFenceValue) {
				previousMainFrameFenceValue = currentMainFrameFenceValue;

				std::vector<RuntimeVirtualTextureNodeRequestTask> rvtNodeRequestTasks;
				ProcessTerrainFeedback(rvtNodeRequestTasks, currentMainFrameFenceValue);

				if (!rvtNodeRequestTasks.empty()) {
					// 压入新的RvtFrame
					uint64_t currFrameFenceExpectedValue = mRvtGraphicsFence->IncrementExpectedValue();
					mRvtFrameTracker->PushCurrentFrame(currFrameFenceExpectedValue);

					// 记录当前帧对应的Gpu命令
					RecordedGpuCommand recordedGpuCommand{};

					// 根据当前请求任务录制GPU命令
					RecordGpuCommand(rvtNodeRequestTasks, recordedGpuCommand);

					// 预留请求任务，以便后续帧完成后的回调处理(mBackComputeFrameTracker->GetFrameIndex()理论上也可以)
					mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).insert(mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).end(), rvtNodeRequestTasks.begin(), rvtNodeRequestTasks.end());

					// 将命令压入队列中
					mRecordedGpuCommands.Push(std::move(recordedGpuCommand));
				}
			}

			// 主线程检测到RvtPageTable的ViewRect发生变动，并将该变动通知给RvtBackend线程，此时，主线程同步等待RvtBackend线程的处理
			if (mRenderer->ConsumeRealRectChanged()) {
				// 对InQueue InLoading InTexture的PageTableNode进行处理
			}

			// 检测塞入的GPU命令是否过载
			if (mRvtFrameTracker->GetUsedSize() == smMaxRvtFrameCount) {
				HANDLE eventHandle = CreateEventEx(nullptr, nullptr, false, EVENT_ALL_ACCESS);

				UINT64 valueToWaitFor = mRvtGraphicsFence->ExpectedValue() - (smMaxRvtFrameCount - 1u);
				mRvtGraphicsFence->SetCompletionEvent(valueToWaitFor, eventHandle);

				// Wait until the GPU hits current fence event is fired.
				WaitForSingleObject(eventHandle, INFINITE);
				CloseHandle(eventHandle);
			}

			// 检测并处理渲染帧是否完成
			mRvtFrameTracker->PopCompletedFrame(mRvtGraphicsFence->CompletedValue());

			if (!mThreadRunning) {
				break;
			}
		}
	}

	void RuntimeVirtualTextureBackend::ProcessTerrainFeedback(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks, uint32_t completedFenceValue) {
		auto& queuedFeedbackReadbacks = mRenderer->mQueuedFeedbackReadbacks;
		auto& terrainFeedbackReadbackBuffers = mRenderer->mTerrainFeedbackReadbackBuffers;

		// 寻找最新鲜的Feedback
		bool feedbackFound = false;
		uint64_t latestFeedbackFenceValue = 0;
		uint32_t targetFeedbackIndex{ 0u };
		for (size_t i = 0; i < queuedFeedbackReadbacks.size(); i++) {
			if (queuedFeedbackReadbacks.at(i).isFresh) {
				uint64_t feedbackFenceValue = queuedFeedbackReadbacks.at(i).renderFrameFenceValue;
				if ((completedFenceValue >= feedbackFenceValue) &&
					((!feedbackFound) || (latestFeedbackFenceValue <= feedbackFenceValue))) {
					feedbackFound = true;
					targetFeedbackIndex = i;
					latestFeedbackFenceValue = feedbackFenceValue;
					// this feedback will either be used or skipped. either way it is "consumed"
					queuedFeedbackReadbacks.at(i).isFresh = false;
				}
			}
		}

		if (!feedbackFound) {
			return;
		}
;
		auto& readbackBuffer = terrainFeedbackReadbackBuffers.at(targetFeedbackIndex);
		uint16_t* pResolvedData = reinterpret_cast<uint16_t*>(readbackBuffer->Map());

		auto& feedbackMapDesc = mRenderer->mTerrainFeedbackMap->GetResourceFormat().GetTextureDesc();
		uint32_t width = feedbackMapDesc.width;
		uint32_t height = feedbackMapDesc.height;

		// rowByteSize的单位是Byte，而我们是2Byte读取的，因此要除以2
		uint32_t rowByteSize = (width * GHL::GetFormatStride(feedbackMapDesc.format) + 0x0ff) & ~0x0ff;
		rowByteSize /= (sizeof(uint16_t) / sizeof(uint8_t));

		auto* tileCache = mRenderer->mNearTerrainRuntimeVirtualTextureAtlasTileCache.get();

		for (uint32_t y = 0u; y < height; y++) {
			for (uint32_t x = 0u; x < rowByteSize;) {

				uint16_t page0PosX = pResolvedData[x++];	// page0PosX
				uint16_t page0PosY = pResolvedData[x++];	// page0PosY
				uint16_t pageLevel = pResolvedData[x++];	// pageLevel
				uint16_t overBound = pResolvedData[x++];	// overBound

				if (overBound == 0u) { continue; }

				// 填充请求队列(注意剔除重复目标)
				auto& currNodeRuntimeState = mRvtPageTables[pageLevel].GetNodeRuntimeState(page0PosX, page0PosY);

				if (currNodeRuntimeState.inReady || currNodeRuntimeState.inQueue || currNodeRuntimeState.inLoading || currNodeRuntimeState.tempFlag) {
					// 该节点对应的资源正在加载
					continue;
				}
				// 该节点对应的资源已在图集上
				else if (!currNodeRuntimeState.inReadyOut && !currNodeRuntimeState.inQueueOut && !currNodeRuntimeState.inLoadingOut && currNodeRuntimeState.inTexture) {
					tileCache->Remove(currNodeRuntimeState.atlasNode);
					tileCache->AddTail(currNodeRuntimeState.atlasNode);
				}
				else {
					// 创建对应的地形请求任务(但不分配图集元素)
					int32_t perSize = (int32_t)std::pow(2, pageLevel);
					RuntimeVirtualTextureNodeRequestTask requestTask{};
					requestTask.nextPageLevel = pageLevel;
					requestTask.nextPagePos.x = page0PosX - page0PosX % perSize;
					requestTask.nextPagePos.y = page0PosY - page0PosY % perSize;
					requestTasks.push_back(requestTask);

					// 剔除重复requestTask
					currNodeRuntimeState.SetTempFlag(true);
				}
			}

			// 转移到纹理的下一行，纹理的每一行除了最后一行都需要256字节对齐
			pResolvedData += rowByteSize;
		}
		readbackBuffer->UnMap();

		// 恢复PageTableNode实时状态
		for (auto& requestTask : requestTasks) {
			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeState(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetTempFlag(false);
		}

		// 安装pageLevel从大到小对请求队列进行排序
		std::sort(requestTasks.begin(), requestTasks.end(),
			[](const RuntimeVirtualTextureNodeRequestTask& taskA, const RuntimeVirtualTextureNodeRequestTask& taskB) {
				return taskA.nextPageLevel < taskB.nextPageLevel;
			}
		);

		// 对请求数量进行限制
		if (requestTasks.size() > mTerrainSetting.smRvtDataLoadedLimit) {
			requestTasks.resize(mTerrainSetting.smRvtDataLoadedLimit);
		}

		// 分配Tile
		for (auto& requestTask : requestTasks) {
			auto* atlasNode = tileCache->GetHead();
			tileCache->Remove(atlasNode);

			// 如果该atlasNode已经负载了一个地形节点的资源，做记录
			if (atlasNode->pageLevel != -1) {
				requestTask.nextPagePos = atlasNode->pagePos;
				requestTask.nextPageLevel = atlasNode->pageLevel;
			}
			// 使用的atlasNode没有负载地形节点
			else {
				requestTask.prevPagePos = Math::Int2{ -1, -1 };
				requestTask.prevPageLevel = -1;
			}

			requestTask.atlasNode = atlasNode;

			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeState(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInReady();
			if (requestTask.prevPageLevel != -1) {
				auto& prevNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeState(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
				prevNodeRuntimeState.SetInReadyOut();
			}
		}
	}

	void RuntimeVirtualTextureBackend::RecordGpuCommand(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand) {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* mainResourceStateTracker = renderEngine->mResourceStateTracker.get();

		std::vector<GpuUpdateRuntimeVirtualTextureAtlasRequest> updateRequests;
		for (auto& requestTask : requestTasks) {
			// 计算tilePos对应的图像空间下的tileRect
			const auto& tileSizeWithPadding = mTerrainSetting.smRvtTileSizeWithPadding;
			const Math::Vector4 tileRectInImageSpace = Math::Vector4{
				requestTask.atlasNode->tilePos.x * (float)tileSizeWithPadding,
				requestTask.atlasNode->tilePos.y * (float)tileSizeWithPadding,
				(float)tileSizeWithPadding,
				(float)tileSizeWithPadding
			};

			float powNumber = std::pow(2, requestTask.nextPageLevel);
			const auto& currRvtRealRect = mRenderer->GetRvtRealRect();
			float paddingEffect = powNumber * mTerrainSetting.smWorldMeterSizePerPaddingInPage0Level;
			Math::Vector4 tileRectInWorldSpace = Math::Vector4{
				currRvtRealRect.x + (mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber * requestTask.nextPagePos.x) - paddingEffect,
				currRvtRealRect.y - (mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber * requestTask.nextPagePos.y) + paddingEffect,
				mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber + 2.0f * paddingEffect,
				mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber + 2.0f * paddingEffect
			};

			// 地块矩形
			const Math::Vector2 terrainMeterSize = mTerrainSetting.smTerrainMeterSize;
			Math::Vector4 terrainRectInWorldSpace = Math::Vector4{
				-mTerrainSetting.smTerrainMeterSize / 2.0f,
				mTerrainSetting.smTerrainMeterSize / 2.0f,
				terrainMeterSize,
				terrainMeterSize
			};

			// 将tileRectInWorldSpace与terrainRectInWorldSpace相重叠
			float tileRectInWorldSpaceOverlappedXMin = std::max(tileRectInWorldSpace.x, terrainRectInWorldSpace.x);
			float tileRectInWorldSpaceOverlappedXMax = std::min(tileRectInWorldSpace.x + tileRectInWorldSpace.z, terrainRectInWorldSpace.x + terrainRectInWorldSpace.z);
			float tileRectInWorldSpaceOverlappedZMin = std::min(tileRectInWorldSpace.y, terrainRectInWorldSpace.y);
			float tileRectInWorldSpaceOverlappedZMax = std::max(tileRectInWorldSpace.y - tileRectInWorldSpace.w, terrainRectInWorldSpace.y - terrainRectInWorldSpace.w);
			Math::Vector4 tileRectInWorldSpaceOverlapped = Math::Vector4{
				tileRectInWorldSpaceOverlappedXMin,
				tileRectInWorldSpaceOverlappedZMin,
				tileRectInWorldSpaceOverlappedXMax - tileRectInWorldSpaceOverlappedXMin,
				tileRectInWorldSpaceOverlappedZMin - tileRectInWorldSpaceOverlappedZMax
			};

			// 计算tileRect在WorldSpace与ImageSpace之间的缩放关系
			float scaleFactorWidth = tileRectInImageSpace.z / tileRectInWorldSpace.z;
			float scaleFactorHeight = tileRectInImageSpace.w / tileRectInWorldSpace.w;

			Math::Vector4 tileRectInImageSpaceOverlapped = Math::Vector4{
				tileRectInImageSpace.x + (tileRectInWorldSpaceOverlapped.x - tileRectInWorldSpace.x) * scaleFactorWidth,
				tileRectInImageSpace.y + (tileRectInWorldSpace.y - tileRectInWorldSpaceOverlapped.y) * scaleFactorHeight,
				tileRectInWorldSpaceOverlapped.z * scaleFactorWidth,
				tileRectInWorldSpaceOverlapped.w * scaleFactorHeight
			};

			// tileRect与terrainRect的长宽缩放与原点（左上角）平移
			// 此处代码见图解
			Math::Vector4 scaleOffset = Math::Vector4{
				tileRectInWorldSpaceOverlapped.z / terrainRectInWorldSpace.z,
				tileRectInWorldSpaceOverlapped.w / terrainRectInWorldSpace.w,
				(tileRectInWorldSpaceOverlapped.x - terrainRectInWorldSpace.x) / terrainRectInWorldSpace.z,
				(terrainRectInWorldSpace.y - tileRectInWorldSpaceOverlapped.y) / terrainRectInWorldSpace.w
			};

			// 此处代码见图解
			Math::Vector2 nowScale = Math::Vector2(terrainRectInWorldSpace.z / mTerrainSetting.smWorldMeterSizePerTiledTexture, terrainRectInWorldSpace.w / mTerrainSetting.smWorldMeterSizePerTiledTexture);
			Math::Vector4 tileOffset = Math::Vector4(nowScale.x * scaleOffset.x, nowScale.y * scaleOffset.y, nowScale.x * scaleOffset.z, nowScale.y * scaleOffset.w);

			// 构建mvpMatrix，用于将渲染结果输出到纹理图集对应的位置上
			float l = tileRectInImageSpaceOverlapped.x * 2.0f / mTerrainSetting.smRvtAtlasTextureSize - 1.0f;
			float r = (tileRectInImageSpaceOverlapped.x + tileRectInImageSpaceOverlapped.z) * 2.0f / mTerrainSetting.smRvtAtlasTextureSize - 1.0f;
			float b = tileRectInImageSpaceOverlapped.y * 2.0f / mTerrainSetting.smRvtAtlasTextureSize - 1.0f;
			float t = (tileRectInImageSpaceOverlapped.y + tileRectInImageSpaceOverlapped.w) * 2.0f / mTerrainSetting.smRvtAtlasTextureSize - 1.0f;
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

			GpuUpdateRuntimeVirtualTextureAtlasRequest updateRequest{};
			updateRequest.mvpMatrix = mvpMatrix;
			updateRequest.tileOffset = tileOffset;
			updateRequest.blendOffset = scaleOffset;
			updateRequests.emplace_back(std::move(updateRequest));
		}

		if (updateRequests.empty()) {
			return;
		}

		// 录制GraphicsCommandList，用于更新RuntimeVirtualTextureAtlas
		{
			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateRuntimeVirtualTextureAtlas");

			uint16_t width = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);
			uint16_t height = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);

			mUpdateRuntimeVirtualTextureAtlasPassData.drawRequestBufferIndex = mUpdateRuntimeVirtualTextureAtlasRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVirtualTextureAtlasPassData.terrainAlbedoTextureArrayIndex = mRenderer->GetNearTerrainAlbedoArray()->GetTextureArray().Get()->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVirtualTextureAtlasPassData.terrainNormalTextureArrayIndex = mRenderer->GetNearTerrainNormalArray()->GetTextureArray().Get()->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVirtualTextureAtlasPassData.terrainRoughnessTextureArrayIndex;
			mUpdateRuntimeVirtualTextureAtlasPassData.terrainSplatMapIndex;

			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRuntimeVirtualTextureAtlasPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRuntimeVirtualTextureAtlasPassData, sizeof(UpdateRuntimeVirtualTextureAtlasPassData));

			auto* terrainAlbedoAtlas = mRenderer->GetNearTerrainRvtAlbedoAtlas();
			auto* terrainNormalAtlas = mRenderer->GetNearTerrainRvtNormalAtlas();
			// 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mUpdateRuntimeVirtualTextureAtlasRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mUpdateRuntimeVirtualTextureAtlasRequestBuffer, 0u, updateRequests.data(), updateRequests.size() * sizeof(GpuUpdateRuntimeVirtualTextureAtlasRequest));
		
			barrierBatch = commandBuffer.TransitionImmediately(mUpdateRuntimeVirtualTextureAtlasRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += mainResourceStateTracker->TransitionImmediately(terrainAlbedoAtlas->GetTextureAtlas(), GHL::EResourceState::RenderTarget);
			barrierBatch += mainResourceStateTracker->TransitionImmediately(terrainNormalAtlas->GetTextureAtlas(), GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetRenderTargets({ terrainAlbedoAtlas->GetTextureAtlas(), terrainNormalAtlas->GetTextureAtlas() });
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState("UpdatePhysicalTexture");
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			// commandBuffer.SetVertexBuffer(0u, quadMesh->GetVertexBuffer());
			// commandBuffer.SetIndexBuffer(quadMesh->GetIndexBuffer());
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			// commandBuffer.DrawIndexedInstanced(quadMesh->GetIndexCount(), updateRequests.size(), 0u, 0u, 0u);

			commandBuffer.PIXEndEvent();

			commandList->Close();
		}

		// 录制GraphicsCommandList，用于更新LookupPageTable
		{

		}
	}

	void RuntimeVirtualTextureBackend::SetupFrameCompletedCallBack() {
		// 添加FrameCompletedCallback
		mRvtFrameTracker->AddFrameCompletedCallBack([this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
			this->OnFrameCompleted(attribute.frameIndex);
		});
	}

	void RuntimeVirtualTextureBackend::OnFrameCompleted(uint8_t frameIndex) {
		auto* rvtTextureAtlasTileCache = mRenderer->mNearTerrainRuntimeVirtualTextureAtlasTileCache.get();

		auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks.at(frameIndex);
		for (auto& requestTask : reservedRequestTasks) {
			// 更新节点实时状态
			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeState(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInTexture();
			currNodeRuntimeState.atlasNode = requestTask.atlasNode;

			// 更新AtlasNode负载
			requestTask.atlasNode->pageLevel = requestTask.nextPageLevel;
			requestTask.atlasNode->pagePos = requestTask.nextPagePos;
			rvtTextureAtlasTileCache->AddTail(requestTask.atlasNode);
		}
		reservedRequestTasks.clear();
	}

	void RuntimeVirtualTextureBackend::CreateGraphicsObject() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageFactory = renderEngine->mDStorageFactory.get();

		auto  shaderPath = renderEngine->smEngineShaderPath;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();

		// 创建图形API对象并设置回调函数
		{
			mRvtResourceStateTracker = std::make_unique<Renderer::ResourceStateTracker>();

			mRvtGraphicsQueue = std::make_unique<GHL::CopyQueue>(device);
			mRvtGraphicsQueue->SetDebugName("RuntimeVT_GraphicsQueue");
			mRvtGraphicsFence = std::make_unique<GHL::Fence>(device);
			mRvtGraphicsFence->SetDebugName("RuntimeVT_GraphicsFence");
			mRvtFrameTracker = std::make_unique<Renderer::RingFrameTracker>(smMaxRvtFrameCount);
			mRvtLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mRvtFrameTracker.get());
			mRvtCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mRvtFrameTracker.get());
		}

		// 创建RequestBuffer
		{
			auto* rvtAlbedoMapAtlas = mRenderer->mNearTerrainRvtAlbedoAtlas.get();

			BufferDesc _UpdateRuntimeVirtualTextureAtlasRequestBufferDesc{};
			_UpdateRuntimeVirtualTextureAtlasRequestBufferDesc.stride = sizeof(GpuUpdateRuntimeVirtualTextureAtlasRequest);
			_UpdateRuntimeVirtualTextureAtlasRequestBufferDesc.size = _UpdateRuntimeVirtualTextureAtlasRequestBufferDesc.stride * rvtAlbedoMapAtlas->GetTileCount();
			_UpdateRuntimeVirtualTextureAtlasRequestBufferDesc.usage = GHL::EResourceUsage::Default;
			_UpdateRuntimeVirtualTextureAtlasRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_UpdateRuntimeVirtualTextureAtlasRequestBufferDesc.initialState = GHL::EResourceState::Common;
			_UpdateRuntimeVirtualTextureAtlasRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mUpdateRuntimeVirtualTextureAtlasRequestBuffer = resourceAllocator->Allocate(device, _UpdateRuntimeVirtualTextureAtlasRequestBufferDesc, descriptorAllocator, nullptr);
			mUpdateRuntimeVirtualTextureAtlasRequestBuffer->SetDebugName("UpdateRuntimeVirtualTextureAtlasRequestBuffer");
			mRvtResourceStateTracker->StartTracking(mUpdateRuntimeVirtualTextureAtlasRequestBuffer);

			BufferDesc _UpdateLookupPageTableRequestBufferDesc{};
			_UpdateLookupPageTableRequestBufferDesc.stride = sizeof(GpuUpdateLookupPageTableRequest);
			_UpdateLookupPageTableRequestBufferDesc.size = _UpdateLookupPageTableRequestBufferDesc.stride * rvtAlbedoMapAtlas->GetTileCount();
			_UpdateLookupPageTableRequestBufferDesc.usage = GHL::EResourceUsage::Default;
			_UpdateLookupPageTableRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_UpdateLookupPageTableRequestBufferDesc.initialState = GHL::EResourceState::Common;
			_UpdateLookupPageTableRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mUpdateLookupPageTableRequestBuffer = resourceAllocator->Allocate(device, _UpdateLookupPageTableRequestBufferDesc, descriptorAllocator, nullptr);
			mUpdateLookupPageTableRequestBuffer->SetDebugName("UpdateLookupPageTableRequestBuffer");
			mRvtResourceStateTracker->StartTracking(mUpdateLookupPageTableRequestBuffer);
		}

		// 创建着色器程序
		{
			shaderManger->CreateGraphicsShader(smUpdateRuntimeVirtualTextureAtlasSN,
				[&](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = shaderPath + "TerrainRenderer/RuntimeVirtualTextureAtlasUpdater.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						mRenderer->GetNearTerrainRvtAlbedoAtlas()->GetDxgiFormat(),		// AlbedoMapAtlas
						mRenderer->GetNearTerrainRvtNormalAtlas()->GetDxgiFormat()		// NormalMapAtlas
					};
				}
			);

			shaderManger->CreateGraphicsShader(smUpdateLookupPageTableMapSN,
				[&](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = shaderPath + "TerrainRenderer/RuntimeVirtualTextureLookupPageTableUpdater.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						DXGI_FORMAT_R16G16B16A16_UINT	// LookupPageTableMap
					};
				}
			);
		}

		// 设置帧完成回调函数
		SetupFrameCompletedCallBack();
	}

}