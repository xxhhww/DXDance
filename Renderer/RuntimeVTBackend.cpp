#include "Renderer/RuntimeVTBackend.h"
#include "Renderer/RuntimeVTAtlas.h"
#include "Renderer/RuntimeVTAtlasTileCache.h"
#include "Renderer/TerrainTextureArray.h"
#include "Renderer/TerrainTiledTexture.h"
#include "Renderer/RenderEngine.h"
#include "Renderer/Misc.h"

#include "Tools/Assert.h"

namespace Renderer {

	RuntimeVTBackend::RuntimeVTBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting)
	: mRenderer(renderer)
	, mTerrainSetting(terrainSetting) 
	, mRvtPageTables(mRenderer->mRuntimeVTPageTables) {
		mReservedTerrainNodeRequestTasks.resize(smMaxRvtFrameCount);

		mHasPreloaded = ::CreateEvent(nullptr, FALSE, FALSE, nullptr);
		ASSERT_FORMAT(mHasPreloaded != nullptr, "CreateEvent Failed");

		CreateGraphicsObject();

		// 启动线程
		mThread = std::thread([this]() {
				this->BackendThread();
			}
		);
	}

	RuntimeVTBackend::~RuntimeVTBackend() {
		mThreadRunning = false;
	}

	void RuntimeVTBackend::Preload() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceStorage = renderEngine->mPipelineResourceStorage;
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		auto* runtimeVTAtlasTileCache = mRenderer->GetRuntimeVTAtlasTileCache();

		mRuntimeVTRealRect = mRenderer->GetRuntimeVTRealRect();

		// 加载MaxPageLevel
		uint32_t maxPageLevel = mTerrainSetting.smRvtMaxPageLevel;
		int32_t powNumber = std::pow(2, maxPageLevel);
		int32_t nodeCountPerAxisInMaxPageLevel = mTerrainSetting.smRvtTileCountPerAxisInPage0Level / powNumber;

		// 填充请求任务，并分配atlasNode
		std::vector<RuntimeVTNodeRequestTask> requestTasks;
		for (int32_t i = 0; i < nodeCountPerAxisInMaxPageLevel; i++) {
			for (int32_t j = 0; j < nodeCountPerAxisInMaxPageLevel; j++) {
				auto* atlasNode = runtimeVTAtlasTileCache->GetHead();
				runtimeVTAtlasTileCache->Remove(atlasNode);

				RuntimeVTNodeRequestTask requestTask{};
				requestTask.atlasNode = atlasNode;
				requestTask.nextPagePos.x = i;
				requestTask.nextPagePos.y = j;
				requestTask.nextPageLevel = maxPageLevel;
				requestTask.prevPagePos.x = -1;
				requestTask.prevPagePos.y = -1;
				requestTask.prevPageLevel = -1;
				requestTask.runtimeVTRealRect = mRuntimeVTRealRect;
				requestTasks.push_back(requestTask);

				auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
				currNodeRuntimeState.SetInReady();
			}
		}

		// 录制GraphicsQueue
		{
			mRvtGraphicsFence->IncrementExpectedValue();
			mRvtFrameTracker->PushCurrentFrame(mRvtGraphicsFence->ExpectedValue());
		}

		// 录制UpdateRuntimeVTAtlas命令
		{
			std::vector<GpuUpdateRuntimeVTAtlasRequest> updateRequests;
			ProduceGpuUpdateRuntimeVTAtlasRequests(requestTasks, updateRequests);

			auto& terrainAlbedoArray    = mRenderer->GetNearTerrainAlbedoArray()->GetTextureArray();
			auto& terrainNormalArray    = mRenderer->GetNearTerrainNormalArray()->GetTextureArray();
			auto& terrainTiledSplatMap  = mRenderer->GetTerrainTiledSplatMap()->GetTiledTexture();
			auto& runtimeVTAlbedoAtlas  = mRenderer->GetRuntimeVTAlbedoAtlas()->GetTextureAtlas();
			auto& runtimeVTNormalAtlas  = mRenderer->GetRuntimeVTNormalAtlas()->GetTextureAtlas();

			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("PreLoadRuntimeVTAtlas");

			uint16_t width = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);
			uint16_t height = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);

			mUpdateRuntimeVTAtlasPassData.drawRequestBufferIndex = mUpdateRuntimeVTAtlasRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainAlbedoTextureArrayIndex = terrainAlbedoArray->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainNormalTextureArrayIndex = terrainNormalArray->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainRoughnessTextureArrayIndex;
			mUpdateRuntimeVTAtlasPassData.terrainSplatMapIndex = terrainTiledSplatMap->GetSRDescriptor()->GetHeapIndex();

			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRuntimeVTAtlasPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRuntimeVTAtlasPassData, sizeof(UpdateRuntimeVTAtlasPassData));

			// 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch += commandBuffer.TransitionImmediately(mUpdateRuntimeVTAtlasRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mUpdateRuntimeVTAtlasRequestBuffer, 0u, updateRequests.data(), updateRequests.size() * sizeof(GpuUpdateRuntimeVTAtlasRequest));

			barrierBatch =  commandBuffer.TransitionImmediately(mUpdateRuntimeVTAtlasRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += resourceStateTracker->TransitionImmediately(runtimeVTAlbedoAtlas, GHL::EResourceState::RenderTarget);
			barrierBatch += resourceStateTracker->TransitionImmediately(runtimeVTNormalAtlas, GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.SetRenderTargets({ runtimeVTAlbedoAtlas, runtimeVTNormalAtlas });
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smUpdateRuntimeVTAtlasSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, updateRequests.size(), 0u, 0u, 0u);

			barrierBatch =  resourceStateTracker->TransitionImmediately(runtimeVTAlbedoAtlas, GHL::EResourceState::PixelShaderAccess);
			barrierBatch += resourceStateTracker->TransitionImmediately(runtimeVTNormalAtlas, GHL::EResourceState::PixelShaderAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.PIXEndEvent();
			commandList->Close();
			mRvtGraphicsQueue->ExecuteCommandList(commandList->D3DCommandList());
		}

		// 录制RuntimeVTPageTable命令
		{
			std::vector<GpuUpdateRuntimeVTPageTableRequest> updateRequests;
			ProduceGpuUpdateRuntimeVTPageTableRequests(requestTasks, updateRequests);

			auto& runtimeVTPageTableMap = mRenderer->GetRuntimeVTPageTableMap();

			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("PreLoadRuntimeVTPageTable");

			uint16_t width = static_cast<uint16_t>(mTerrainSetting.smRvtTileCountPerAxisInPage0Level);
			uint16_t height = static_cast<uint16_t>(mTerrainSetting.smRvtTileCountPerAxisInPage0Level);

			mUpdateRuntimeVTPageTablePassData.drawRequestBufferIndex = mUpdateRuntimeVTPageTableRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTPageTablePassData.runtimeVTPageTableCopyIndex = mRuntimeVTPageTableCopy->GetUADescriptor()->GetHeapIndex();
			mUpdateRuntimeVTPageTablePassData.preLoad = 1;
			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRuntimeVTPageTablePassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRuntimeVTPageTablePassData, sizeof(UpdateRuntimeVTPageTablePassData));

			// Copy RuntimeVTPageTable & 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch += commandBuffer.TransitionImmediately(mUpdateRuntimeVTPageTableRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mUpdateRuntimeVTPageTableRequestBuffer, 0u, updateRequests.data(), updateRequests.size() * sizeof(GpuUpdateRuntimeVTPageTableRequest));

			barrierBatch =  resourceStateTracker->TransitionImmediately(runtimeVTPageTableMap, GHL::EResourceState::RenderTarget);
			barrierBatch += commandBuffer.TransitionImmediately(mRuntimeVTPageTableCopy, GHL::EResourceState::UnorderedAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mUpdateRuntimeVTPageTableRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.SetRenderTarget(runtimeVTPageTableMap);
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smUpdateRuntimeVTPageTableSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, updateRequests.size(), 0u, 0u, 0u);

			barrierBatch =  resourceStateTracker->TransitionImmediately(runtimeVTPageTableMap, GHL::EResourceState::PixelShaderAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.PIXEndEvent();
			commandList->Close();
			mRvtGraphicsQueue->ExecuteCommandList(commandList->D3DCommandList());
		}

		// 等待任务完成
		{
			mRvtGraphicsQueue->SignalFence(*mRvtGraphicsFence.get());
			mRvtGraphicsFence->Wait();
			mRvtFrameTracker->PopCompletedFrame(mRvtGraphicsFence->CompletedValue());
		}

		// GPU任务完成，更新CPU中PageTableNode的驻留状态
		for (auto& requestTask : requestTasks) {
			// 更新节点实时状态
			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInTexture();
			currNodeRuntimeState.atlasNode = requestTask.atlasNode;

			// 更新AtlasNode负载
			requestTask.atlasNode->pageLevel = requestTask.nextPageLevel;
			requestTask.atlasNode->pagePos = mRvtPageTables[requestTask.nextPageLevel].GetTransformedXY(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			requestTask.atlasNode->pageOffset = mRvtPageTables[requestTask.nextPageLevel].GetPageOffset();
			requestTask.atlasNode->runtimeVTRealRect = mRuntimeVTRealRect;
			requestTask.atlasNode->pagePosInRealRect = requestTask.nextPagePos;
			runtimeVTAtlasTileCache->AddTail(requestTask.atlasNode);
		}

		// 设置帧完成回调函数
		SetupFrameCompletedCallBack();

		// 加载完成后通知后台线程，启动资源调度线程
		::SetEvent(mHasPreloaded);
	}

	void RuntimeVTBackend::OnFrameLoading(uint32_t frameIndex) {
		auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks.at(frameIndex);

		for (auto& requestTask : reservedRequestTasks) {
			// 更新节点实时状态
			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInLoading();
			if (requestTask.prevPageLevel != -1) {
				auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
				prevNodeRuntimeState.SetInLoadingOut();
			}
		}
	}

	void RuntimeVTBackend::BackendThread() {
		// 等待预先加载完成后启动任务
		::WaitForSingleObject(mHasPreloaded, INFINITE);

		auto* renderEngine = mRenderer->mRenderEngine;
		auto* mainRenderFrameFence = renderEngine->mRenderFrameFence.get();
		uint64_t previousMainFrameFenceValue = 0u;
		bool runtimeVTRealRectChanged = false;

		while (mThreadRunning) {
			uint64_t currentMainFrameFenceValue = mainRenderFrameFence->CompletedValue();

			// 新的主渲染帧完成
			if (previousMainFrameFenceValue != currentMainFrameFenceValue) {
				previousMainFrameFenceValue = currentMainFrameFenceValue;

				std::vector<RuntimeVTNodeRequestTask> rvtNodeRequestTasks;
				runtimeVTRealRectChanged = ProcessTerrainFeedback(rvtNodeRequestTasks, currentMainFrameFenceValue);

				if (!rvtNodeRequestTasks.empty() && !runtimeVTRealRectChanged) {
					// 压入新的RvtFrame
					uint64_t currFrameFenceExpectedValue = mRvtGraphicsFence->IncrementExpectedValue();
					mRvtFrameTracker->PushCurrentFrame(currFrameFenceExpectedValue);

					// 记录当前帧对应的Gpu命令
					RecordedGpuCommand recordedGpuCommand{};
					recordedGpuCommand.graphicsQueue = mRvtGraphicsQueue.get();
					recordedGpuCommand.graphicsFence = mRvtGraphicsFence.get();
					recordedGpuCommand.graphicsFenceExpectedValue = currFrameFenceExpectedValue;
					recordedGpuCommand.updateRuntimeVTAtlasCommandList = nullptr;
					recordedGpuCommand.updateRuntimeVTPageTableCommandList = nullptr;
					recordedGpuCommand.updateRuntimeVTAtlasInRealRectChangedCommandList = nullptr;
					recordedGpuCommand.makeRuntimeVTPageTableInvalidCommandList = nullptr;
					recordedGpuCommand.updateRuntimeVTPageTableInRealRectChangedCommandList = nullptr;
					recordedGpuCommand.frameIndex = mRvtFrameTracker->GetCurrFrameIndex();

					// 根据当前请求任务录制GPU命令
					RecordGpuCommand(rvtNodeRequestTasks, recordedGpuCommand);
					// 预留请求任务，以便后续帧完成后的回调处理(mBackComputeFrameTracker->GetFrameIndex()理论上也可以)
					mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).insert(mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).end(), rvtNodeRequestTasks.begin(), rvtNodeRequestTasks.end());

					// 将命令压入队列中
					mRecordedGpuCommands.Push(std::move(recordedGpuCommand));
				}
			}

			// 主线程检测到RuntimeVTRealRectChanged事件，并将该事件通知给RuntimeVTBackend线程，此时，主线程此时同步等待RuntimeVTBackend线程的处理
			if (mRenderer->CheckRuntimeVTRealRectChanged() || runtimeVTRealRectChanged) {
				runtimeVTRealRectChanged = false;

				mRuntimeVTRealRect = mRenderer->GetRuntimeVTRealRect();

				// 处理RuntimeVTRealRectChanged
				ProcessRuntimeVTRealRectChanged();

				// 压入新的RvtFrame
				uint64_t currFrameFenceExpectedValue = mRvtGraphicsFence->IncrementExpectedValue();
				mRvtFrameTracker->PushCurrentFrame(currFrameFenceExpectedValue);

				// 记录当前帧对应的Gpu命令
				RecordedGpuCommand recordedGpuCommand{};
				recordedGpuCommand.graphicsQueue = mRvtGraphicsQueue.get();
				recordedGpuCommand.graphicsFence = mRvtGraphicsFence.get();
				recordedGpuCommand.graphicsFenceExpectedValue = currFrameFenceExpectedValue;
				recordedGpuCommand.updateRuntimeVTAtlasCommandList = nullptr;
				recordedGpuCommand.updateRuntimeVTPageTableCommandList = nullptr;
				recordedGpuCommand.updateRuntimeVTAtlasInRealRectChangedCommandList = nullptr;
				recordedGpuCommand.makeRuntimeVTPageTableInvalidCommandList = nullptr;
				recordedGpuCommand.updateRuntimeVTPageTableInRealRectChangedCommandList = nullptr;
				recordedGpuCommand.frameIndex = mRvtFrameTracker->GetCurrFrameIndex();

				// 录制命令
				std::vector<RuntimeVTNodeRequestTask> rvtNodeRequestTasks;
				RecordedGpuCommandInRealRectChanged(recordedGpuCommand, rvtNodeRequestTasks);

				// 预留请求任务，以便后续帧完成后的回调处理(mBackComputeFrameTracker->GetFrameIndex()理论上也可以)
				mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).insert(mReservedTerrainNodeRequestTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).end(), rvtNodeRequestTasks.begin(), rvtNodeRequestTasks.end());

				// 将命令压入队列中
				mRecordedGpuCommandsInRealRectChanged.Push(std::move(recordedGpuCommand));

				// 通知主线程
				mRenderer->SetRuntimeVTRealRectChangedCompletedEvnet();
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

	bool RuntimeVTBackend::ProcessTerrainFeedback(std::vector<RuntimeVTNodeRequestTask>& requestTasks, uint32_t completedFenceValue) {
		auto& queuedFeedbackReadbacks = mRenderer->mQueuedFeedbackReadbacks;
		auto& terrainFeedbackReadbackBuffers = mRenderer->mTerrainFeedbackReadbackBuffers;
		
		// 寻找最新鲜的且RuntimeVTRealRect一致的Feedback
		bool feedbackFound = false;
		uint64_t latestFeedbackFenceValue = 0;
		uint32_t targetFeedbackIndex{ 0u };
		for (size_t i = 0; i < queuedFeedbackReadbacks.size(); i++) {
			if (queuedFeedbackReadbacks.at(i).isFresh && queuedFeedbackReadbacks.at(i).runtimeVTRealRect == mRuntimeVTRealRect) {
				uint64_t feedbackFenceValue = queuedFeedbackReadbacks.at(i).renderFrameFenceValue;
				if ((completedFenceValue >= feedbackFenceValue) && ((!feedbackFound) || (latestFeedbackFenceValue <= feedbackFenceValue))) {
					feedbackFound = true;
					targetFeedbackIndex = i;
					latestFeedbackFenceValue = feedbackFenceValue;
					// this feedback will either be used or skipped. either way it is "consumed"
					queuedFeedbackReadbacks.at(i).isFresh = false;
				}
			}
		}

		if (!feedbackFound) {
			return false;
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

		auto* tileCache = mRenderer->GetRuntimeVTAtlasTileCache();

		for (uint32_t y = 0u; y < height; y++) {
			for (uint32_t x = 0u; x < rowByteSize;) {
				// 每一次都判断是否发生RuntimeVTRealRectChanged事件
				if (mRenderer->CheckRuntimeVTRealRectChanged()) {

					// 恢复PageTableNode实时状态
					for (auto& requestTask : requestTasks) {
						auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
						currNodeRuntimeState.SetTempFlag(false);
					}

					return true;
				}

				uint16_t page0PosX = pResolvedData[x++];	// page0PosX
				uint16_t page0PosY = pResolvedData[x++];	// page0PosY
				uint16_t pageLevel = pResolvedData[x++];	// pageLevel
				uint16_t overBound = pResolvedData[x++];	// overBound

				if (overBound == 0u) { continue; }

				// 将Page0Pos转换到当前Level下的PagePos
				int32_t perSize = (int32_t)std::pow(2, pageLevel);
				int32_t pagePosX = page0PosX / perSize;
				int32_t pagePosY = page0PosY / perSize;

				// 填充请求队列(注意剔除重复目标)
				auto& currNodeRuntimeState = mRvtPageTables[pageLevel].GetNodeRuntimeStateTransformed((uint32_t)pagePosX, (uint32_t)pagePosY);


				/*
				if (currNodeRuntimeState.tempFlag) {
					// 该节点对应的资源正在加载
					continue;
				}

				// 创建对应的地形请求任务(但不分配图集元素)
				RuntimeVTNodeRequestTask requestTask{};
				requestTask.nextPageLevel = (int32_t)pageLevel;
				requestTask.nextPagePos.x = pagePosX;
				requestTask.nextPagePos.y = pagePosY;
				requestTask.runtimeVTRealRect = mRuntimeVTRealRect;
				requestTasks.push_back(requestTask);

				// 剔除重复requestTask
				currNodeRuntimeState.SetTempFlag(true);
				*/


				if (currNodeRuntimeState.inReady || currNodeRuntimeState.inQueue || currNodeRuntimeState.inLoading || currNodeRuntimeState.tempFlag) {
					// 该节点对应的资源正在加载
					continue;
				}
				// 该节点对应的资源已在图集上
				else if (!currNodeRuntimeState.inReadyOut && !currNodeRuntimeState.inQueueOut && !currNodeRuntimeState.inLoadingOut && currNodeRuntimeState.inTexture) {
					if (currNodeRuntimeState.atlasNode == nullptr) {
						int32_t i = 0;
					}
					tileCache->Remove(currNodeRuntimeState.atlasNode);
					tileCache->AddTail(currNodeRuntimeState.atlasNode);
				}
				else {
					// 创建对应的地形请求任务(但不分配图集元素)
					RuntimeVTNodeRequestTask requestTask{};
					requestTask.nextPageLevel = (int32_t)pageLevel;
					requestTask.nextPagePos.x = pagePosX;
					requestTask.nextPagePos.y = pagePosY;
					requestTask.runtimeVTRealRect = mRuntimeVTRealRect;
					requestTasks.push_back(requestTask);

					// 剔除重复requestTask
					currNodeRuntimeState.SetTempFlag(true);
				}
			}

			// 转移到纹理的下一行，纹理的每一行除了最后一行都需要256字节对齐
			pResolvedData += rowByteSize;
		}
		readbackBuffer->UnMap();

		// 保持最高级别的PageLevel一直处于驻留状态
		auto& runtimeVTPageTable = mRvtPageTables.back();
		auto& maxPageLevelNodeRuntimeStates = runtimeVTPageTable.GetNodeRuntimeStates();
		for (auto& rows : maxPageLevelNodeRuntimeStates) {
			for (auto& currNodeRuntimeState : rows) {
				if (currNodeRuntimeState.atlasNode != nullptr) {
					tileCache->Remove(currNodeRuntimeState.atlasNode);
					tileCache->AddTail(currNodeRuntimeState.atlasNode);
				}
			}
		}

		// 恢复PageTableNode实时状态
		for (auto& requestTask : requestTasks) {
			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetTempFlag(false);
		}

		// 按照pageLevel从大到小对请求队列进行排序
		std::sort(requestTasks.begin(), requestTasks.end(),
			[](const RuntimeVTNodeRequestTask& taskA, const RuntimeVTNodeRequestTask& taskB) {
				return taskA.nextPageLevel > taskB.nextPageLevel;
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
				requestTask.prevPagePos = atlasNode->pagePos;
				requestTask.prevPageLevel = atlasNode->pageLevel;
			}
			// 使用的atlasNode没有负载地形节点
			else {
				requestTask.prevPagePos = Math::Int2{ -1, -1 };
				requestTask.prevPageLevel = -1;
			}
			
			requestTask.atlasNode = atlasNode;

			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInReady();
			if (requestTask.prevPageLevel != -1) {
				auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
				bool inTExture = prevNodeRuntimeState.inTexture;
				prevNodeRuntimeState.SetInReadyOut();
				if (prevNodeRuntimeState.atlasNode == nullptr) {
					int32_t i = 0;
				}
			}
		}

		return false;
	}

	void RuntimeVTBackend::ProduceGpuUpdateRuntimeVTAtlasRequests(std::vector<RuntimeVTNodeRequestTask>& requestTasks, std::vector<GpuUpdateRuntimeVTAtlasRequest>& updateRequests) {
		for (auto& requestTask : requestTasks) {
			// 计算tilePos对应的图像空间下的tileRect
			const auto& tileSizeWithPadding = mTerrainSetting.smRvtTileSizeWithPadding;
			const Math::Vector4 tileRectInImageSpace = Math::Vector4{
				(float)requestTask.atlasNode->tilePos.x * (float)tileSizeWithPadding,
				(float)requestTask.atlasNode->tilePos.y * (float)tileSizeWithPadding,
				(float)tileSizeWithPadding,
				(float)tileSizeWithPadding
			};

			float powNumber = std::pow(2, requestTask.nextPageLevel);
			const auto& currRvtRealRect = mRenderer->GetRuntimeVTRealRect();
			float paddingEffect = powNumber * mTerrainSetting.smWorldMeterSizePerPaddingInPage0Level;
			Math::Vector4 tileRectInWorldSpace = Math::Vector4{
				currRvtRealRect.x + (mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber * (float)requestTask.nextPagePos.x) - paddingEffect,
				currRvtRealRect.y - (mTerrainSetting.smWorldMeterSizePerTileInPage0Level * powNumber * (float)requestTask.nextPagePos.y) + paddingEffect,
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

			GpuUpdateRuntimeVTAtlasRequest updateRequest{};
			updateRequest.mvpMatrix = mvpMatrix;
			updateRequest.tileOffset = tileOffset;
			updateRequest.blendOffset = scaleOffset;
			updateRequests.emplace_back(std::move(updateRequest));
		}
	}

	void RuntimeVTBackend::ProduceGpuUpdateRuntimeVTPageTableRequests(std::vector<RuntimeVTNodeRequestTask>& requestTasks, std::vector<GpuUpdateRuntimeVTPageTableRequest>& updateRequests) {
		for (const auto& requestTask : requestTasks) {
			int32_t powNumber = std::pow(2, requestTask.nextPageLevel);
			Math::Int2 pagePosInPage0Level = requestTask.nextPagePos * powNumber;

			GpuUpdateRuntimeVTPageTableRequest updateRequest;
			updateRequest.tilePosX = requestTask.atlasNode->tilePos.x;
			updateRequest.tilePosY = requestTask.atlasNode->tilePos.y;
			updateRequest.pageLevel = requestTask.nextPageLevel;
			updateRequest.rectInPage0Level = Math::Int4{ pagePosInPage0Level.x, pagePosInPage0Level.y, powNumber, powNumber };
			updateRequests.push_back(updateRequest);

			if (requestTask.prevPageLevel != -1) {
				int32_t pageLevel = requestTask.atlasNode->pageLevel;
				int32_t powNumber = std::pow(2, pageLevel);

				// 当前atlasNode上对应的RuntimeVTRealRect可能与当前不同，因此需要将prevPagePos转换到当前RuntimeVTRealRect的视角下的pagePos
				Math::Int2    pageOffsetInPrevRealRect = requestTask.atlasNode->pageOffset;
				Math::Int2    pagePosInPrevRealRect = requestTask.atlasNode->pagePosInRealRect;

				Math::Vector4 currRealRect = mRuntimeVTRealRect;
				Math::Int2    pageOffsetInCurrRealRect = mRvtPageTables[pageLevel].GetPageOffset();

				Math::Int2    offsetInCurrPageLevel = pageOffsetInCurrRealRect - pageOffsetInPrevRealRect;
				Math::Int2    pagePosInCurrPageLevel = pagePosInPrevRealRect + offsetInCurrPageLevel;
				Math::Int2    pagePosInCurrPage0Level = pagePosInCurrPageLevel * powNumber;

				RuntimeVTPageTableNodeRuntimeState prevNodeRuntimeStates = mRvtPageTables[pageLevel].GetNodeRuntimeStateTransformed(pagePosInCurrPageLevel.x, pagePosInCurrPageLevel.y);

				while (pageLevel <= mRenderer->GetMaxPageLevel() && (prevNodeRuntimeStates.inLoadingOut || prevNodeRuntimeStates.inQueueOut || prevNodeRuntimeStates.inReadyOut || !prevNodeRuntimeStates.inTexture)) {
					pageLevel++;

					powNumber = std::pow(2, pageLevel);
					pagePosInCurrPageLevel.x = pagePosInCurrPage0Level.x / powNumber;
					pagePosInCurrPageLevel.y = pagePosInCurrPage0Level.y / powNumber;
					prevNodeRuntimeStates = mRvtPageTables[pageLevel].GetNodeRuntimeStateTransformed(pagePosInCurrPageLevel.x, pagePosInCurrPageLevel.y);
				}

				if (prevNodeRuntimeStates.inLoadingOut || prevNodeRuntimeStates.inQueueOut || prevNodeRuntimeStates.inReadyOut) {
					int32_t i = 0;
				}

				updateRequest.tilePosX = prevNodeRuntimeStates.atlasNode->tilePos.x;
				updateRequest.tilePosY = prevNodeRuntimeStates.atlasNode->tilePos.y;
				updateRequest.pageLevel = prevNodeRuntimeStates.pageLevel;
				updateRequest.rectInPage0Level = Math::Int4{ pagePosInCurrPage0Level.x, pagePosInCurrPage0Level.y, powNumber, powNumber };
				updateRequests.push_back(updateRequest);
			}
		}

		// 排序(从大到小)
		std::sort(updateRequests.begin(), updateRequests.end(),
			[](const GpuUpdateRuntimeVTPageTableRequest& requestA, const GpuUpdateRuntimeVTPageTableRequest& requestB) {
				return requestA.pageLevel > requestB.pageLevel;
			});

		// 生成MVPMatrix
		for (auto& updateRequest : updateRequests) {
			float size = (float)updateRequest.rectInPage0Level.z / (float)mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			updateRequest.mvpMatrix = Math::Matrix4{
				Math::Vector3{ updateRequest.rectInPage0Level.x / (float)mTerrainSetting.smRvtTileCountPerAxisInPage0Level, updateRequest.rectInPage0Level.y / (float)mTerrainSetting.smRvtTileCountPerAxisInPage0Level, 0.0f },
				Math::Quaternion{},
				Math::Vector3{ size, size, size }
			}.Transpose();
		}
	}

	void RuntimeVTBackend::RecordGpuCommand(std::vector<RuntimeVTNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand) {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();

		std::vector<GpuUpdateRuntimeVTAtlasRequest> updateRequests;
		ProduceGpuUpdateRuntimeVTAtlasRequests(requestTasks, updateRequests);

		if (updateRequests.empty()) {
			return;
		}

		// 录制GraphicsCommandList，用于更新RuntimeVTAtlas
		{
			auto& terrainAlbedoArray   = mRenderer->GetNearTerrainAlbedoArray()->GetTextureArray();
			auto& terrainNormalArray   = mRenderer->GetNearTerrainNormalArray()->GetTextureArray();
			auto& terrainTiledSplatMap = mRenderer->GetTerrainTiledSplatMap()->GetTiledTexture();
			auto& runtimeVTAlbedoAtlas = mRenderer->GetRuntimeVTAlbedoAtlas()->GetTextureAtlas();
			auto& runtimeVTNormalAtlas = mRenderer->GetRuntimeVTNormalAtlas()->GetTextureAtlas();

			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateRuntimeVTAtlas");

			uint16_t width = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);
			uint16_t height = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);

			mUpdateRuntimeVTAtlasPassData.drawRequestBufferIndex         = mUpdateRuntimeVTAtlasRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainAlbedoTextureArrayIndex = terrainAlbedoArray->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainNormalTextureArrayIndex = terrainNormalArray->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainRoughnessTextureArrayIndex;
			mUpdateRuntimeVTAtlasPassData.terrainSplatMapIndex           = terrainTiledSplatMap->GetSRDescriptor()->GetHeapIndex();

			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRuntimeVTAtlasPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRuntimeVTAtlasPassData, sizeof(UpdateRuntimeVTAtlasPassData));

			// 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mUpdateRuntimeVTAtlasRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mUpdateRuntimeVTAtlasRequestBuffer, 0u, updateRequests.data(), updateRequests.size() * sizeof(GpuUpdateRuntimeVTAtlasRequest));
		
			barrierBatch = commandBuffer.TransitionImmediately(mUpdateRuntimeVTAtlasRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTAlbedoAtlas, GHL::EResourceState::PixelShaderAccess, GHL::EResourceState::RenderTarget} };
			barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTNormalAtlas, GHL::EResourceState::PixelShaderAccess, GHL::EResourceState::RenderTarget} };
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.SetRenderTargets({ runtimeVTAlbedoAtlas, runtimeVTNormalAtlas });
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smUpdateRuntimeVTAtlasSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, updateRequests.size(), 0u, 0u, 0u);

			barrierBatch =  GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTAlbedoAtlas, GHL::EResourceState::RenderTarget, GHL::EResourceState::PixelShaderAccess} };
			barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTNormalAtlas, GHL::EResourceState::RenderTarget, GHL::EResourceState::PixelShaderAccess} };
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.PIXEndEvent();
			commandList->Close();
			recordedGpuCommand.updateRuntimeVTAtlasCommandList = commandList.Get();
		}

		// 生成GpuUpdateRuntimeVTPageTableRequests;
		std::vector<GpuUpdateRuntimeVTPageTableRequest> updateRuntimeVTPageTableRequests;
		for (const auto& requestTask : requestTasks) {
			int32_t powNumber = std::pow(2, requestTask.nextPageLevel);
			Math::Int2 pagePosInPage0Level = requestTask.nextPagePos * powNumber;

			GpuUpdateRuntimeVTPageTableRequest updateRequest;
			updateRequest.tilePosX = requestTask.atlasNode->tilePos.x;
			updateRequest.tilePosY = requestTask.atlasNode->tilePos.y;
			updateRequest.pageLevel = requestTask.nextPageLevel;
			updateRequest.rectInPage0Level = Math::Int4{ pagePosInPage0Level.x, pagePosInPage0Level.y, powNumber, powNumber };
			updateRuntimeVTPageTableRequests.push_back(updateRequest);

			if (requestTask.prevPageLevel != -1) {
				int32_t pageLevel = requestTask.atlasNode->pageLevel;
				int32_t powNumber = std::pow(2, pageLevel);

				// 当前atlasNode上对应的RuntimeVTRealRect可能与当前不同，因此需要将prevPagePos转换到当前RuntimeVTRealRect的视角下的pagePos
				const Math::Vector4 prevRuntimeVTRealRect = requestTask.atlasNode->runtimeVTRealRect;
				const Math::Vector4 currRuntimeVTRealRect = mRuntimeVTRealRect;

				const Math::Int2 prevRuntimeVTRealRectCenter = Math::Int2{
					(int32_t)prevRuntimeVTRealRect.x + (int32_t)prevRuntimeVTRealRect.z,
					(int32_t)prevRuntimeVTRealRect.y - (int32_t)prevRuntimeVTRealRect.w
				};

				const Math::Int2 currRuntimeVTRealRectCenter = Math::Int2{
					(int32_t)currRuntimeVTRealRect.x + (int32_t)currRuntimeVTRealRect.z,
					(int32_t)currRuntimeVTRealRect.y - (int32_t)currRuntimeVTRealRect.w
				};
				 
				const Math::Int2 offsetInPage0Level = (currRuntimeVTRealRectCenter - prevRuntimeVTRealRectCenter) / (int32_t)mTerrainSetting.smWorldMeterSizePerTileInPage0Level;
				const Math::Int2 offsetInCurrPageLevel = offsetInPage0Level / powNumber;

				Math::Int2 pagePosInCurrRealRect = requestTask.atlasNode->pagePosInRealRect; + offsetInCurrPageLevel;
				Math::Int2 pagePosInCurrPage0Level = pagePosInCurrRealRect * powNumber;

				RuntimeVTPageTableNodeRuntimeState prevNodeRuntimeStates = mRvtPageTables[pageLevel].GetNodeRuntimeStateTransformed(pagePosInCurrRealRect.x, pagePosInCurrRealRect.y);

				while (pageLevel <= mRenderer->GetMaxPageLevel() && (prevNodeRuntimeStates.inLoadingOut || prevNodeRuntimeStates.inQueueOut || prevNodeRuntimeStates.inReadyOut || !prevNodeRuntimeStates.inTexture)) {
					pageLevel ++;

					pagePosInCurrRealRect.x = pagePosInCurrRealRect.x / 2;
					pagePosInCurrRealRect.y = pagePosInCurrRealRect.y / 2;
					prevNodeRuntimeStates = mRvtPageTables[pageLevel].GetNodeRuntimeStateTransformed(pagePosInCurrRealRect.x, pagePosInCurrRealRect.y);
				}

				if (prevNodeRuntimeStates.inLoadingOut || prevNodeRuntimeStates.inQueueOut || prevNodeRuntimeStates.inReadyOut) {
					int32_t i = 0;
				}

				updateRequest.tilePosX = prevNodeRuntimeStates.atlasNode->tilePos.x;
				updateRequest.tilePosY = prevNodeRuntimeStates.atlasNode->tilePos.y;
				updateRequest.pageLevel = prevNodeRuntimeStates.pageLevel;
				updateRequest.rectInPage0Level = Math::Int4{ pagePosInCurrPage0Level.x, pagePosInCurrPage0Level.y, powNumber, powNumber };
				updateRuntimeVTPageTableRequests.push_back(updateRequest);
			}
		}

		// 排序(从大到小)
		std::sort(updateRuntimeVTPageTableRequests.begin(), updateRuntimeVTPageTableRequests.end(),
			[](const GpuUpdateRuntimeVTPageTableRequest& requestA, const GpuUpdateRuntimeVTPageTableRequest& requestB) {
				return requestA.pageLevel > requestB.pageLevel;
			});

		// 生成MVPMatrix
		for (auto& updateRequest : updateRuntimeVTPageTableRequests) {
			float size = (float)updateRequest.rectInPage0Level.z / (float)mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			updateRequest.mvpMatrix = Math::Matrix4{
				Math::Vector3{ updateRequest.rectInPage0Level.x / (float)mTerrainSetting.smRvtTileCountPerAxisInPage0Level, updateRequest.rectInPage0Level.y / (float)mTerrainSetting.smRvtTileCountPerAxisInPage0Level, 0.0f },
				Math::Quaternion{},
				Math::Vector3{ size, size, size }
			}.Transpose();
		}

		// 录制GraphicsCommandList，用于更新PageTable
		{
			auto& runtimeVTPageTableMap = mRenderer->GetRuntimeVTPageTableMap();

			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateRuntimeVTPageTable");

			uint16_t width = static_cast<uint16_t>(mTerrainSetting.smRvtTileCountPerAxisInPage0Level);
			uint16_t height = static_cast<uint16_t>(mTerrainSetting.smRvtTileCountPerAxisInPage0Level);

			mUpdateRuntimeVTPageTablePassData.drawRequestBufferIndex      = mUpdateRuntimeVTPageTableRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTPageTablePassData.runtimeVTPageTableCopyIndex = mRuntimeVTPageTableCopy->GetUADescriptor()->GetHeapIndex();
			mUpdateRuntimeVTPageTablePassData.preLoad = 0;
			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRuntimeVTPageTablePassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRuntimeVTPageTablePassData, sizeof(UpdateRuntimeVTPageTablePassData));

			// Copy RuntimeVTPageTable & 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTPageTableMap, GHL::EResourceState::PixelShaderAccess, GHL::EResourceState::CopySource} };
			barrierBatch += commandBuffer.TransitionImmediately(mRuntimeVTPageTableCopy, GHL::EResourceState::CopyDestination);
			barrierBatch += commandBuffer.TransitionImmediately(mUpdateRuntimeVTPageTableRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.CopyResource(mRuntimeVTPageTableCopy, runtimeVTPageTableMap);
			commandBuffer.UploadBufferRegion(mUpdateRuntimeVTPageTableRequestBuffer, 0u, updateRuntimeVTPageTableRequests.data(), updateRuntimeVTPageTableRequests.size() * sizeof(GpuUpdateRuntimeVTPageTableRequest));

			barrierBatch =  commandBuffer.TransitionImmediately(mRuntimeVTPageTableCopy, GHL::EResourceState::UnorderedAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mUpdateRuntimeVTPageTableRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTPageTableMap, GHL::EResourceState::CopySource, GHL::EResourceState::RenderTarget} };
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.SetRenderTarget(runtimeVTPageTableMap);
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smUpdateRuntimeVTPageTableSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, updateRuntimeVTPageTableRequests.size(), 0u, 0u, 0u);

			barrierBatch = GHL::ResourceBarrierBatch{ GHL::TransitionBarrier{runtimeVTPageTableMap, GHL::EResourceState::RenderTarget, GHL::EResourceState::PixelShaderAccess} };
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.PIXEndEvent();
			commandList->Close();
			recordedGpuCommand.updateRuntimeVTPageTableCommandList = commandList.Get();
		}
	}

	void RuntimeVTBackend::ProcessRuntimeVTRealRectChanged() {

		auto* runtimeVTAtlasTileCache = mRenderer->GetRuntimeVTAtlasTileCache();
		
		// 将已经录制好但没有提交的任务全部清空
		RecordedGpuCommand recordedGpuCommand{};
		while (!mRecordedGpuCommands.Empty()) {
			mRecordedGpuCommands.TryPop(recordedGpuCommand);

			// CommandList不执行，只执行SignalFence操作
			recordedGpuCommand.graphicsQueue->SignalFence(*recordedGpuCommand.graphicsFence, recordedGpuCommand.graphicsFenceExpectedValue);

			auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks[recordedGpuCommand.frameIndex];
			for (auto& requestTask : reservedRequestTasks) {
				auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
				currNodeRuntimeState.SetOutTexture();
				currNodeRuntimeState.atlasNode = nullptr;
				if (requestTask.prevPageLevel != -1) {
					// 孩子们，我回来了！
					auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
					prevNodeRuntimeState.SetInTexture();
				}

				ASSERT_FORMAT(requestTask.atlasNode->prev == nullptr);
				ASSERT_FORMAT(requestTask.atlasNode->next == nullptr);
				runtimeVTAtlasTileCache->AddHead(requestTask.atlasNode);
			}
			reservedRequestTasks.clear();
		}

		mRvtGraphicsFence->Wait();
		mRvtFrameTracker->PopCompletedFrame(mRvtGraphicsFence->CompletedValue());
		
		/*
		// 假定预留的任务全都没有做完，恢复节点实时状态(帧倒序恢复)
		for (int32_t i = mReservedTerrainNodeRequestTasks.size() - 1; i >= 0; i--) {
			auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks[i];
			for (auto& requestTask : reservedRequestTasks) {
				requestTask.atlasNode->pageLevel = -1;
				requestTask.atlasNode->pagePos = Math::Int2{ -1, -1 };

				auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
				currNodeRuntimeState.SetOutTexture();
				currNodeRuntimeState.atlasNode = nullptr;
				if (requestTask.prevPageLevel != -1) {
					auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
					prevNodeRuntimeState.SetOutTexture();
					prevNodeRuntimeState.atlasNode = nullptr;
				}

				ASSERT_FORMAT(requestTask.atlasNode->prev == nullptr);
				ASSERT_FORMAT(requestTask.atlasNode->next == nullptr);
				runtimeVTAtlasTileCache->AddHead(requestTask.atlasNode);
			}
			reservedRequestTasks.clear();
		}
		*/

		auto& runtimeVTPageTables = mRenderer->GetRuntimeVTPageTables();
		std::vector<RuntimeVTPageTableNodeRuntimeState*> changedRuntimeVTPageTableNodeRuntimeStates;

		for (auto& runtimeVTPageTable : runtimeVTPageTables) {
			runtimeVTPageTable.OnRuntimeVTRealRectChanged(mRenderer->GetRuntimeVTRealRectOffsetInPage0Level(), changedRuntimeVTPageTableNodeRuntimeStates);
		}

		for (auto& changedNodeRuntimeState : changedRuntimeVTPageTableNodeRuntimeStates) {
			// 在changedRuntimeVTPageTableNodeRuntimeStates中可能存在重复值，需要再进行nullptr判断
			if (changedNodeRuntimeState->atlasNode != nullptr) {
				runtimeVTAtlasTileCache->Remove(changedNodeRuntimeState->atlasNode);
				runtimeVTAtlasTileCache->AddHead(changedNodeRuntimeState->atlasNode);
				changedNodeRuntimeState->atlasNode->pageLevel = -1;
				changedNodeRuntimeState->atlasNode->pagePos = Math::Int2{ -1, -1 };
				changedNodeRuntimeState->atlasNode = nullptr;
				changedNodeRuntimeState->SetOutTexture();
			}
		}
	}

	void RuntimeVTBackend::RecordedGpuCommandInRealRectChanged(RecordedGpuCommand& recordedGpuCommand, std::vector<RuntimeVTNodeRequestTask>& requestTasks) {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();
		auto* resourceStateTracker = renderEngine->mResourceStateTracker.get();

		auto& runtimeVTPageTableMap = mRenderer->GetRuntimeVTPageTableMap();
		auto& runtimeVTPageTableMapDesc = runtimeVTPageTableMap->GetResourceFormat().GetTextureDesc();

		auto* runtimeVTAtlasTileCache = mRenderer->GetRuntimeVTAtlasTileCache();

		const Math::Int2 offsetInPage0Level = mRenderer->GetRuntimeVTRealRectOffsetInPage0Level();

		// 计算非法PageLevel
		uint32_t invalidPageLevelBeg = 0u;
		uint32_t invalidPageLevelEnd = mTerrainSetting.smRvtMaxPageLevel;
		uint32_t powNumber = std::pow(2, invalidPageLevelBeg);
		while (offsetInPage0Level.x % powNumber == 0 && offsetInPage0Level.y % powNumber == 0) {
			invalidPageLevelBeg++;
			powNumber = std::pow(2, invalidPageLevelBeg);
		}

		// 计算非法区域
		/*
		offsetInPage0Level = int2(0, 32)
		invalidRegionX = int2(0, 255)
		invalidRegionY = int2(224, 255)

		offsetInPage0Level = int2(0, -32)
		invalidRegionX = int2(0, 255)
		invalidRegionY = int2(0, 31)

		offsetInPage0Level = int2(32, 0)
		invalidRegionX = int2(0, 31)
		invalidRegionY = int2(0, 255)

		offsetInPage0Level = int2(-32, 0)
		invalidRegionX = int2(224, 255)
		invalidRegionY = int2(0, 255)
		*/

		int32_t invalidRegionXBeg = 0;
		int32_t invalidRegionXEnd = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
		int32_t invalidRegionYBeg = 0;
		int32_t invalidRegionYEnd = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;

		if (offsetInPage0Level.x > 0) {
			invalidRegionXEnd = offsetInPage0Level.x - 1;
		}
		else if (offsetInPage0Level.x < 0) {
			invalidRegionXBeg = invalidRegionXEnd + offsetInPage0Level.x + 1;
		}

		if (offsetInPage0Level.y > 0) {
			invalidRegionYBeg = invalidRegionYEnd - offsetInPage0Level.y + 1;
		}
		else if (offsetInPage0Level.y < 0) {
			invalidRegionYEnd = -(offsetInPage0Level.y + 1);
		}

		// 计算RequestTasks，加载MaxPageLevel
		{
			uint32_t maxPageLevel = mTerrainSetting.smRvtMaxPageLevel;
			int32_t  powNumber = std::pow(2, maxPageLevel);
			int32_t  nodeCountPerAxisInMaxPageLevel = mTerrainSetting.smRvtTileCountPerAxisInPage0Level / powNumber;

			for (int32_t i = 0; i < nodeCountPerAxisInMaxPageLevel; i++) {
				for (int32_t j = 0; j < nodeCountPerAxisInMaxPageLevel; j++) {
					auto* atlasNode = runtimeVTAtlasTileCache->GetHead();
					runtimeVTAtlasTileCache->Remove(atlasNode);

					RuntimeVTNodeRequestTask requestTask{};
					requestTask.atlasNode = atlasNode;
					requestTask.nextPagePos.x = i;
					requestTask.nextPagePos.y = j;
					requestTask.nextPageLevel = maxPageLevel;

					if (atlasNode->pageLevel != -1) {
						requestTask.prevPagePos = atlasNode->pagePos;
						requestTask.prevPageLevel = atlasNode->pageLevel;
					}
					requestTask.runtimeVTRealRect = mRuntimeVTRealRect;
					requestTasks.push_back(requestTask);

					auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
					currNodeRuntimeState.SetInReady();
					if (requestTask.prevPageLevel != -1) {
						auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
						bool inTExture = prevNodeRuntimeState.inTexture;
						prevNodeRuntimeState.SetInReadyOut();
						if (prevNodeRuntimeState.atlasNode == nullptr) {
							int32_t i = 0;
						}
					}
				}
			}
		}

		// 录制RuntimeVTAtlasPass
		{
			std::vector<GpuUpdateRuntimeVTAtlasRequest> updateRequests;
			ProduceGpuUpdateRuntimeVTAtlasRequests(requestTasks, updateRequests);

			auto& terrainAlbedoArray    = mRenderer->GetNearTerrainAlbedoArray()->GetTextureArray();
			auto& terrainNormalArray    = mRenderer->GetNearTerrainNormalArray()->GetTextureArray();
			auto& terrainTiledSplatMap  = mRenderer->GetTerrainTiledSplatMap()->GetTiledTexture();
			auto& runtimeVTAlbedoAtlas  = mRenderer->GetRuntimeVTAlbedoAtlas()->GetTextureAtlas();
			auto& runtimeVTNormalAtlas  = mRenderer->GetRuntimeVTNormalAtlas()->GetTextureAtlas();

			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateRuntimeVTAtlasInRealRectChanged");

			uint16_t width = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);
			uint16_t height = static_cast<uint16_t>(mTerrainSetting.smRvtAtlasTextureSize);

			mUpdateRuntimeVTAtlasPassData.drawRequestBufferIndex = mUpdateRuntimeVTAtlasRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainAlbedoTextureArrayIndex = terrainAlbedoArray->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainNormalTextureArrayIndex = terrainNormalArray->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTAtlasPassData.terrainRoughnessTextureArrayIndex;
			mUpdateRuntimeVTAtlasPassData.terrainSplatMapIndex = terrainTiledSplatMap->GetSRDescriptor()->GetHeapIndex();

			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRuntimeVTAtlasPassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRuntimeVTAtlasPassData, sizeof(UpdateRuntimeVTAtlasPassData));

			// 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mUpdateRuntimeVTAtlasRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mUpdateRuntimeVTAtlasRequestBuffer, 0u, updateRequests.data(), updateRequests.size() * sizeof(GpuUpdateRuntimeVTAtlasRequest));

			barrierBatch =  commandBuffer.TransitionImmediately(mUpdateRuntimeVTAtlasRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += resourceStateTracker->TransitionImmediately(runtimeVTAlbedoAtlas, GHL::EResourceState::RenderTarget);
			barrierBatch += resourceStateTracker->TransitionImmediately(runtimeVTNormalAtlas, GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.SetRenderTargets({ runtimeVTAlbedoAtlas, runtimeVTNormalAtlas });
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smUpdateRuntimeVTAtlasSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, updateRequests.size(), 0u, 0u, 0u);

			barrierBatch =  resourceStateTracker->TransitionImmediately(runtimeVTPageTableMap, GHL::EResourceState::UnorderedAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mRuntimeVTPageTableCopy, GHL::EResourceState::UnorderedAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.PIXEndEvent();
			commandList->Close();

			recordedGpuCommand.updateRuntimeVTAtlasInRealRectChangedCommandList = commandList.Get();
		}

		// 录制RuntimeVTPageTableChangedPass
		{
			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("MakeRuntimeVTPageTableInvalid");

			mRuntimeVTPageTableChangedPass1Data.runtimeVTPageTableMapIndex = runtimeVTPageTableMap->GetUADescriptor()->GetHeapIndex();
			mRuntimeVTPageTableChangedPass1Data.maxPageLevel = mTerrainSetting.smRvtMaxPageLevel;
			mRuntimeVTPageTableChangedPass1Data.invalidPageLevelBeg = invalidPageLevelBeg;
			mRuntimeVTPageTableChangedPass1Data.invalidPageLevelEnd = invalidPageLevelEnd;
			mRuntimeVTPageTableChangedPass1Data.invalidRegionXBeg = invalidRegionXBeg;
			mRuntimeVTPageTableChangedPass1Data.invalidRegionXEnd = invalidRegionXEnd;
			mRuntimeVTPageTableChangedPass1Data.invalidRegionYBeg = invalidRegionYBeg;
			mRuntimeVTPageTableChangedPass1Data.invalidRegionYEnd = invalidRegionYEnd;

			auto passDataAlloc1 = mRvtLinearBufferAllocator->Allocate(sizeof(RuntimeVTPageTableChangedPass1Data));
			memcpy(passDataAlloc1.cpuAddress, &mRuntimeVTPageTableChangedPass1Data, sizeof(RuntimeVTPageTableChangedPass1Data));

			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch += resourceStateTracker->TransitionImmediately(runtimeVTPageTableMap, GHL::EResourceState::UnorderedAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetComputeRootSignature();
			commandBuffer.SetComputeRootCBV(1u, passDataAlloc1.gpuAddress);
			commandBuffer.SetComputePipelineState(smRuntimeVTPageTableChangedPass1SN);
			commandBuffer.Dispatch(runtimeVTPageTableMapDesc.width / smThreadSizeInGroup, runtimeVTPageTableMapDesc.height / smThreadSizeInGroup, 1u);

			mRuntimeVTPageTableChangedPass2Data.srcRuntimeVTPageTableMapIndex = runtimeVTPageTableMap->GetSRDescriptor()->GetHeapIndex();
			mRuntimeVTPageTableChangedPass2Data.dstRuntimeVTPageTableMapIndex = mRuntimeVTPageTableCopy->GetUADescriptor()->GetHeapIndex();
			mRuntimeVTPageTableChangedPass2Data.pixelOffset = offsetInPage0Level;
			mRuntimeVTPageTableChangedPass2Data.pageTableMapSize = runtimeVTPageTableMapDesc.width;

			auto passDataAlloc2 = mRvtLinearBufferAllocator->Allocate(sizeof(RuntimeVTPageTableChangedPass2Data));
			memcpy(passDataAlloc2.cpuAddress, &mRuntimeVTPageTableChangedPass2Data, sizeof(RuntimeVTPageTableChangedPass2Data));

			barrierBatch = resourceStateTracker->TransitionImmediately(runtimeVTPageTableMap, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mRuntimeVTPageTableCopy, GHL::EResourceState::UnorderedAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetComputeRootSignature();
			commandBuffer.SetComputeRootCBV(1u, passDataAlloc2.gpuAddress);
			commandBuffer.SetComputePipelineState(smRuntimeVTPageTableChangedPass2SN);
			commandBuffer.Dispatch(runtimeVTPageTableMapDesc.width / smThreadSizeInGroup, runtimeVTPageTableMapDesc.height / smThreadSizeInGroup, 1u);

			barrierBatch = resourceStateTracker->TransitionImmediately(runtimeVTPageTableMap, GHL::EResourceState::CopyDestination);
			barrierBatch += commandBuffer.TransitionImmediately(mRuntimeVTPageTableCopy, GHL::EResourceState::CopySource);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.CopyResource(runtimeVTPageTableMap, mRuntimeVTPageTableCopy);

			commandBuffer.PIXEndEvent();
			commandList->Close();

			recordedGpuCommand.makeRuntimeVTPageTableInvalidCommandList = commandList.Get();
		}

		{
			std::vector<GpuUpdateRuntimeVTPageTableRequest> updateRequests;
			ProduceGpuUpdateRuntimeVTPageTableRequests(requestTasks, updateRequests);

			auto& runtimeVTPageTableMap = mRenderer->GetRuntimeVTPageTableMap();

			auto commandList = mRvtCommandListAllocator->AllocateGraphicsCommandList();
			auto* descriptorHeap = descriptorAllocator->GetCBSRUADescriptorHeap().D3DDescriptorHeap();
			commandList->D3DCommandList()->SetDescriptorHeaps(1u, &descriptorHeap);
			CommandBuffer commandBuffer{ commandList.Get(), shaderManger, mRvtResourceStateTracker.get(), mRvtLinearBufferAllocator.get(), nullptr };
			commandBuffer.PIXBeginEvent("UpdateRuntimeVTPageTableInRealRectChanged");

			uint16_t width = static_cast<uint16_t>(mTerrainSetting.smRvtTileCountPerAxisInPage0Level);
			uint16_t height = static_cast<uint16_t>(mTerrainSetting.smRvtTileCountPerAxisInPage0Level);

			mUpdateRuntimeVTPageTablePassData.drawRequestBufferIndex = mUpdateRuntimeVTPageTableRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdateRuntimeVTPageTablePassData.runtimeVTPageTableCopyIndex = mRuntimeVTPageTableCopy->GetUADescriptor()->GetHeapIndex();
			mUpdateRuntimeVTPageTablePassData.preLoad = 0;
			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdateRuntimeVTPageTablePassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdateRuntimeVTPageTablePassData, sizeof(UpdateRuntimeVTPageTablePassData));

			// Copy RuntimeVTPageTable & 将数据数组上传到GPU中去
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch += commandBuffer.TransitionImmediately(mUpdateRuntimeVTPageTableRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mUpdateRuntimeVTPageTableRequestBuffer, 0u, updateRequests.data(), updateRequests.size() * sizeof(GpuUpdateRuntimeVTPageTableRequest));

			barrierBatch =  resourceStateTracker->TransitionImmediately(runtimeVTPageTableMap, GHL::EResourceState::RenderTarget);
			barrierBatch += commandBuffer.TransitionImmediately(mRuntimeVTPageTableCopy, GHL::EResourceState::UnorderedAccess);
			barrierBatch += commandBuffer.TransitionImmediately(mUpdateRuntimeVTPageTableRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.SetRenderTarget(runtimeVTPageTableMap);
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState(smUpdateRuntimeVTPageTableSN);
			commandBuffer.SetGraphicsRootCBV(1u, passDataAlloc.gpuAddress);
			commandBuffer.SetVertexBuffer(0u, mQuadMeshVertexBuffer);
			commandBuffer.SetIndexBuffer(mQuadMeshIndexBuffer);
			commandBuffer.SetPrimitiveTopology(D3D_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
			commandBuffer.DrawIndexedInstanced(mQuadMeshIndexCount, updateRequests.size(), 0u, 0u, 0u);

			barrierBatch = resourceStateTracker->TransitionImmediately(runtimeVTPageTableMap, GHL::EResourceState::PixelShaderAccess);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.PIXEndEvent();
			commandList->Close();

			recordedGpuCommand.updateRuntimeVTPageTableInRealRectChangedCommandList = commandList.Get();
		}
	}

	void RuntimeVTBackend::SetupFrameCompletedCallBack() {
		// 添加FrameCompletedCallback
		mRvtFrameTracker->AddFrameCompletedCallBack([this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
			this->OnFrameCompleted(attribute.frameIndex);
		});
	}

	void RuntimeVTBackend::OnFrameCompleted(uint8_t frameIndex) {
		auto* runtimeVTAtlasTileCache = mRenderer->GetRuntimeVTAtlasTileCache();

		auto& reservedRequestTasks = mReservedTerrainNodeRequestTasks.at(frameIndex);
		for (auto& requestTask : reservedRequestTasks) {
			// 更新节点实时状态
			auto& currNodeRuntimeState = mRvtPageTables[requestTask.nextPageLevel].GetNodeRuntimeStateTransformed(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			currNodeRuntimeState.SetInTexture();
			currNodeRuntimeState.atlasNode = requestTask.atlasNode;
			if (currNodeRuntimeState.atlasNode == nullptr) {
				int32_t i = 0;
			}
			if (requestTask.prevPageLevel != -1) {
				auto& prevNodeRuntimeState = mRvtPageTables[requestTask.prevPageLevel].GetNodeRuntimeStateDirected(requestTask.prevPagePos.x, requestTask.prevPagePos.y);
				prevNodeRuntimeState.SetOutTexture();
				prevNodeRuntimeState.atlasNode = nullptr;
			}

			// 更新AtlasNode负载
			requestTask.atlasNode->pageLevel = requestTask.nextPageLevel;
			requestTask.atlasNode->pagePos = mRvtPageTables[requestTask.nextPageLevel].GetTransformedXY(requestTask.nextPagePos.x, requestTask.nextPagePos.y);
			requestTask.atlasNode->pageOffset = mRvtPageTables[requestTask.nextPageLevel].GetPageOffset();
			requestTask.atlasNode->runtimeVTRealRect = mRuntimeVTRealRect;
			requestTask.atlasNode->pagePosInRealRect = requestTask.nextPagePos;
			runtimeVTAtlasTileCache->AddTail(requestTask.atlasNode);
		}
		reservedRequestTasks.clear();
	}

	void RuntimeVTBackend::CreateGraphicsObject() {
		auto* renderEngine = mRenderer->mRenderEngine;
		auto* device = renderEngine->mDevice.get();
		auto* dstorageQueue = renderEngine->mDStorageMemQueue.get();
		auto* dstorageFence = renderEngine->mDStorageFence.get();

		auto  shaderPath = renderEngine->smEngineShaderPath;
		auto* shaderManger = renderEngine->mShaderManger.get();
		auto* resourceAllocator = renderEngine->mResourceAllocator.get();
		auto* descriptorAllocator = renderEngine->mDescriptorAllocator.get();

		// 创建图形API对象并设置回调函数
		{
			mRvtResourceStateTracker = std::make_unique<Renderer::ResourceStateTracker>();

			mRvtGraphicsQueue = std::make_unique<GHL::GraphicsQueue>(device);
			mRvtGraphicsQueue->SetDebugName("RuntimeVT_GraphicsQueue");
			mRvtGraphicsFence = std::make_unique<GHL::Fence>(device);
			mRvtGraphicsFence->SetDebugName("RuntimeVT_GraphicsFence");

			mRvtComputeQueue = std::make_unique<GHL::ComputeQueue>(device);
			mRvtComputeQueue->SetDebugName("RuntimeVT_ComputeQueue");
			mRvtComputeFence = std::make_unique<GHL::Fence>(device);
			mRvtComputeFence->SetDebugName("RuntimeVT_ComputeFence");

			mRvtFrameTracker = std::make_unique<Renderer::RingFrameTracker>(smMaxRvtFrameCount);
			mRvtLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mRvtFrameTracker.get());
			mRvtCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mRvtFrameTracker.get());
		}

		// 创建RequestBuffer
		{
			auto* runtimeVTAlbedoAtlas = mRenderer->mRuntimeVTAlbedoAtlas.get();

			BufferDesc _UpdateRuntimeVTAtlasRequestBufferDesc{};
			_UpdateRuntimeVTAtlasRequestBufferDesc.stride = sizeof(GpuUpdateRuntimeVTAtlasRequest);
			_UpdateRuntimeVTAtlasRequestBufferDesc.size = _UpdateRuntimeVTAtlasRequestBufferDesc.stride * runtimeVTAlbedoAtlas->GetTileCount();
			_UpdateRuntimeVTAtlasRequestBufferDesc.usage = GHL::EResourceUsage::Default;
			_UpdateRuntimeVTAtlasRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_UpdateRuntimeVTAtlasRequestBufferDesc.initialState = GHL::EResourceState::Common;
			_UpdateRuntimeVTAtlasRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mUpdateRuntimeVTAtlasRequestBuffer = resourceAllocator->Allocate(device, _UpdateRuntimeVTAtlasRequestBufferDesc, descriptorAllocator, nullptr);
			mUpdateRuntimeVTAtlasRequestBuffer->SetDebugName("UpdateRuntimeVTAtlasRequestBuffer");
			mRvtResourceStateTracker->StartTracking(mUpdateRuntimeVTAtlasRequestBuffer);

			BufferDesc _UpdateRuntimeVTPageTableRequestBufferDesc{};
			_UpdateRuntimeVTPageTableRequestBufferDesc.stride = sizeof(GpuUpdateRuntimeVTPageTableRequest);
			_UpdateRuntimeVTPageTableRequestBufferDesc.size = _UpdateRuntimeVTPageTableRequestBufferDesc.stride * runtimeVTAlbedoAtlas->GetTileCount();
			_UpdateRuntimeVTPageTableRequestBufferDesc.usage = GHL::EResourceUsage::Default;
			_UpdateRuntimeVTPageTableRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
			_UpdateRuntimeVTPageTableRequestBufferDesc.initialState = GHL::EResourceState::Common;
			_UpdateRuntimeVTPageTableRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
			mUpdateRuntimeVTPageTableRequestBuffer = resourceAllocator->Allocate(device, _UpdateRuntimeVTPageTableRequestBufferDesc, descriptorAllocator, nullptr);
			mUpdateRuntimeVTPageTableRequestBuffer->SetDebugName("UpdateRuntimeVTPageTableRequestBuffer");
			mRvtResourceStateTracker->StartTracking(mUpdateRuntimeVTPageTableRequestBuffer);
		}

		// 创建RuntimeVTPageTable副本
		{
			TextureDesc _RuntimeVTPageTableMapDesc{};
			_RuntimeVTPageTableMapDesc.width = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			_RuntimeVTPageTableMapDesc.height = mTerrainSetting.smRvtTileCountPerAxisInPage0Level;
			_RuntimeVTPageTableMapDesc.format = DXGI_FORMAT_R8G8B8A8_UINT;
			_RuntimeVTPageTableMapDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::UnorderedAccess | GHL::EResourceState::CopySource;
			_RuntimeVTPageTableMapDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
			mRuntimeVTPageTableCopy = resourceAllocator->Allocate(device, _RuntimeVTPageTableMapDesc, descriptorAllocator, nullptr);
			mRuntimeVTPageTableCopy->SetDebugName("RuntimeVTPageTableCopy");
			mRvtResourceStateTracker->StartTracking(mRuntimeVTPageTableCopy);
		}

		// 创建QuadMesh
		{
			std::vector<Vertex> vertices;
			vertices.resize(6u);
			vertices[0].position = Math::Vector3{ 0.0f, 1.0f, 0.0f };
			vertices[0].uv = Math::Vector2{ 0.0f, 1.0f };
			vertices[1].position = Math::Vector3{ 0.0f, 0.0f, 0.0f };
			vertices[1].uv = Math::Vector2{ 0.0f, 0.0f };
			vertices[2].position = Math::Vector3{ 1.0f, 0.0f, 0.0f };
			vertices[2].uv = Math::Vector2{ 1.0f, 0.0f };
			vertices[3].position = Math::Vector3{ 1.0f, 1.0f, 0.0f };
			vertices[3].uv = Math::Vector2{ 1.0f, 1.0f };

			std::vector<uint32_t> indices;
			indices.emplace_back(0u);
			indices.emplace_back(2u);
			indices.emplace_back(1u);
			indices.emplace_back(2u);
			indices.emplace_back(0u);
			indices.emplace_back(3u);

			Renderer::BufferDesc vertexBufferDesc{};
			vertexBufferDesc.stride = sizeof(Renderer::Vertex);
			vertexBufferDesc.size = vertexBufferDesc.stride * vertices.size();
			vertexBufferDesc.usage = GHL::EResourceUsage::Default;
			mQuadMeshVertexBuffer = resourceAllocator->Allocate(device, vertexBufferDesc, descriptorAllocator, nullptr);

			Renderer::BufferDesc indexBufferDesc{};
			indexBufferDesc.stride = sizeof(uint32_t);
			indexBufferDesc.size = indexBufferDesc.stride * indices.size();
			indexBufferDesc.usage = GHL::EResourceUsage::Default;
			mQuadMeshIndexBuffer = resourceAllocator->Allocate(device, indexBufferDesc, descriptorAllocator, nullptr);
			mQuadMeshIndexCount = indices.size();

			EnqueueDStorageRequest(dstorageQueue, static_cast<void*>(vertices.data()), vertexBufferDesc.size, mQuadMeshVertexBuffer.Get(), 0u);
			EnqueueDStorageRequest(dstorageQueue, static_cast<void*>(indices.data()), indexBufferDesc.size, mQuadMeshIndexBuffer.Get(), 0u);

			dstorageFence->IncrementExpectedValue();
			dstorageQueue->EnqueueSignal(*dstorageFence);
			dstorageQueue->Submit();
			dstorageFence->Wait();
		}

		// 创建着色器程序
		{
			shaderManger->CreateGraphicsShader(smUpdateRuntimeVTAtlasSN,
				[&](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = shaderPath + "TerrainRenderer/RuntimeVTAtlasUpdater.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						mRenderer->GetRuntimeVTAlbedoAtlas()->GetDxgiFormat(),		// AlbedoMapAtlas
						mRenderer->GetRuntimeVTNormalAtlas()->GetDxgiFormat()		// NormalMapAtlas
					};
				}
			);

			shaderManger->CreateGraphicsShader(smUpdateRuntimeVTPageTableSN,
				[&](GraphicsStateProxy& proxy) {
					proxy.vsFilepath = shaderPath + "TerrainRenderer/RuntimeVTPageTableUpdater.hlsl";
					proxy.psFilepath = proxy.vsFilepath;
					proxy.rasterizerDesc.CullMode = D3D12_CULL_MODE_NONE;
					proxy.depthStencilDesc.DepthEnable = false;
					proxy.renderTargetFormatArray = {
						mRenderer->GetRuntimeVTPageTableMap()->GetResourceFormat().GetTextureDesc().format
					};
				}
			);

			shaderManger->CreateComputeShader(smRuntimeVTPageTableChangedPass1SN,
				[&](ComputeStateProxy& proxy) {
					proxy.csFilepath = shaderPath + "TerrainRenderer/RuntimeVTPageTableChangedPass1.hlsl";
				}
			);

			shaderManger->CreateComputeShader(smRuntimeVTPageTableChangedPass2SN,
				[&](ComputeStateProxy& proxy) {
					proxy.csFilepath = shaderPath + "TerrainRenderer/RuntimeVTPageTableChangedPass2.hlsl";
				}
			);
		}
	}

}