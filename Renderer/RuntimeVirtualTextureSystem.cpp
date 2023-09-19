#include "Renderer/RuntimeVirtualTextureSystem.h"
#include "Renderer/TerrainSystem.h"
#include "Renderer/RenderEngine.h"
#include "Tools/Assert.h"

namespace Renderer {

	struct GPUDrawPhysicalTextureRequest {
	public:
		Math::Vector4 tileRectInWorldSpace;
		Math::Vector4 tileRectInImageSpace;
		Math::Matrix4 mvpMatrix;

		Math::Vector4 blendOffset;
		Math::Vector4 tileOffset;

	public:
		inline GPUDrawPhysicalTextureRequest(const Math::Vector4& wsTileRect, const Math::Vector4& isTileRect, const Math::Matrix4& mvp,
			const Math::Vector4& blendOffset, const Math::Vector4& tileOffset)
			: tileRectInWorldSpace(wsTileRect)
			, tileRectInImageSpace(isTileRect)
			, mvpMatrix(mvp) 
			, blendOffset(blendOffset)
			, tileOffset(tileOffset) {}
	};

	struct GPUDrawPageTableTextureRequest {
	public:
		Math::Vector4 rect;			// Draw��Ŀ������(x,yΪx,y --- z,wΪwidth,height)
		int32_t mipLevel;
		float   pad1;
		Math::Vector2 tilePos;		// ����ֵ

		Math::Matrix4 mvpMatrix;	// ת����ͼƬ�ռ��еľ���

	public:
		inline GPUDrawPageTableTextureRequest(const Math::Vector4& rect, int32_t mipLevel, const Math::Vector2& tilePos)
			: rect(rect)
			, mipLevel(mipLevel)
			, tilePos(tilePos) {}
	};

	RuntimeVirtualTextureSystem::RuntimeVirtualTextureSystem(TerrainSystem* terrainSystem) 
	: mTerrainSystem(terrainSystem)
	, mRenderEngine(terrainSystem->mRenderEngine)
	, mMainShaderManger(mRenderEngine->mShaderManger.get())
	, mMainResourceStateTracker(mRenderEngine->mResourceStateTracker.get())
	, mMainDescriptorAllocator(mRenderEngine->mDescriptorAllocator.get()) 
	, mMaxRvtFrameCount(3u) 
	, mTileSize(256u) 
	, mPaddingSize(4u) 
	, mLimitTaskPerFrame(6u) {

		mCurrRvtRect = Math::Vector4{
			0.0f - terrainSystem->worldMeterSize.x / 2.0f,
			0.0f - terrainSystem->worldMeterSize.y / 2.0f,
			terrainSystem->worldMeterSize.x,
			terrainSystem->worldMeterSize.y
		};

		mLoadingTasks.resize(mMaxRvtFrameCount);

		// ��������������
		uint32_t pixelPerMeter = 128u;	// ÿ�׵�����ֵ
		float tileCountPerMeter = (float)pixelPerMeter / (float)mTileSize;	// ÿ�׵�Tile�ĸ���

		mTileCountPerAxis = (uint32_t)mTerrainSystem->worldMeterSize.x * tileCountPerMeter;	// ������������������
		mTileCount = mTileCountPerAxis * mTileCountPerAxis;
		mVirtualTextureSize = mTileCountPerAxis * mTileSize;

		// ����2ֱ��mTileCountPerAxis�����ٱ�2����
		mMaxMipLevel = 0u;
		uint32_t tempCount = mTileCountPerAxis;
		while (tempCount != 1 && tempCount % 2 == 0) {
			mMaxMipLevel++;
			tempCount /= 2;
		}

		// ��ʼ��PageTable����������
		mPageTable = std::make_unique<PageTable>(mMaxMipLevel, mTileCountPerAxis);
		mRvtPhysicalTexture = std::make_unique<RvtPhysicalTexture>(this);

		// ��ʼ��ͼ�ζ���
		InitializeGraphicsObject(mTerrainSystem);

		// ���������߳�
		mProcessThread = std::thread([this]() {
			this->UpdateThread();
			}
		);
	}

	RuntimeVirtualTextureSystem::~RuntimeVirtualTextureSystem() {
		mThreadRunning = false;
		mProcessThread.join();
	}

	void RuntimeVirtualTextureSystem::UpdateThread() {
		auto* mainRenderFrameFence = mRenderEngine->mRenderFrameFence.get();
		uint64_t previousMainFrameFenceValue = 0u;

		bool moreTask = false;

		while (mThreadRunning) {
			uint64_t currentMainFrameFenceValue = mainRenderFrameFence->CompletedValue();

			// �µ�����Ⱦ֡���
			if (previousMainFrameFenceValue != currentMainFrameFenceValue) {
				previousMainFrameFenceValue = currentMainFrameFenceValue;
				
				// ѹ���µ�Rvt֡
				mRvtFrameFence->IncrementExpectedValue();
				mRvtFrameTracker->PushCurrentFrame(mRvtFrameFence->ExpectedValue());

				// ��Feedback��Readback���д���
				// �ڴ�������лὫ��ʱ��δʹ�õ�Tile�ӻ����б��з����ȥ��Ҳ����˵�����б����˸ı�
				ProcessFeedback(currentMainFrameFenceValue);

				// ��סGPU��Դ״̬
				LockGPUResource();

				// ���������б�������Ӧ��GPU��������GPU������
				UpdatePhysicalTexturePass();

				// �������б�ĸ����ύ��GPU��ȥ
				UpdatePageTableTexturePass();

				mRvtGrahpicsQueue->SignalFence(*mRvtFrameFence.get());

				// ����
				UnlockGPUResource();
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

	void RuntimeVirtualTextureSystem::ProcessFeedback(uint64_t completedFenceValue) {
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
			// �µ�Feedback�����һЩԭ����Ҫ���ص�Tile��ò��ٱ���Ҫ��
			// ��������ЩTile����Ⱦ�����Ѿ����ύ��GPU��ȥ����һ�������ǲ��账����������LRU Cache�ı䶯�����޳�
			// �����е�����һ����Tile����Ⱦ����δ���ύ������Щ���������
			ClearTaskQueue();
			TextureWrap& terrainFeedbackMap = mTerrainSystem->mTerrainFeedbackMap;
			auto& textureDesc = terrainFeedbackMap->GetResourceFormat().GetTextureDesc();

			BufferWrap& terrainReadbackBuffer = terrainReadbackBuffers.at(targetFeedbackIndex);
			uint16_t* pResolvedData = reinterpret_cast<uint16_t*>(terrainReadbackBuffer->Map());
			
			uint32_t width = textureDesc.width;
			uint32_t height = textureDesc.height;

			// rowByteSize�ĵ�λ��Byte����������2Byte��ȡ�ģ����Ҫ����2
			uint32_t rowByteSize = (width * GHL::GetFormatStride(textureDesc.format) + 0x0ff) & ~0x0ff;
			rowByteSize /= (sizeof(uint16_t) / sizeof(uint8_t));

			for (uint32_t y = 0u; y < height; y++) {
				for (uint32_t x = 0u; x < rowByteSize;) {
					
					uint16_t r = pResolvedData[x++];	// page0PosX
					uint16_t g = pResolvedData[x++];	// page0PosY
					uint16_t b = pResolvedData[x++];	// mipLevel
					uint16_t a = pResolvedData[x++];

					if (a == 0u) {
						continue;
					}

					if (g >= 1536 && g < 2048)
					{
						int i = 32;
					}
					// ����һ��Tile
					DistributeTile(r, g, b);
				}

				// ת�Ƶ��������һ�У������ÿһ�г������һ�ж���Ҫ256�ֽڶ���
				pResolvedData += rowByteSize;
			}
			terrainReadbackBuffer->UnMap();
		}
		return;
	}

	// ������������
	void RuntimeVirtualTextureSystem::UpdatePhysicalTexturePass() {
		if (mTasks.empty()) {
			return;
		}
		// ����mipLevel��������(mipLevel��С����)
		std::sort(mTasks.begin(), mTasks.end(),
			[](const Task& taskA, const Task& taskB) {
				return taskA.nextMipLevel < taskB.nextMipLevel;
			}
		);

		// ÿ��ֻ�������Ƹ�����Task
		auto& tileCache = mRvtPhysicalTexture->GetTileCache();
		int32_t count = mLimitTaskPerFrame;
		std::vector<GPUDrawPhysicalTextureRequest> gpuDrawRequests;
		while (count > 0 && mTasks.size() > 0u) {
			Task task = mTasks.back();
			mTasks.pop_back();
			// Task task = mTasks.at(mTasks.size() - count);

			// Request Head Node
			auto* head = tileCache.GetHead();
			if (head->page0Pos.x != -1 && head->page0Pos.y != -1 && head->mipLevel != -1) {
				task.prevPage0Pos = head->page0Pos;
				task.prevMipLevel = head->mipLevel;
			}
			// take out head node
			tileCache.Remove(head);
			task.node = head;

			// �޸�״̬
			if (task.prevMipLevel != -1 && task.prevPage0Pos != Math::Int2{ -1, -1 }) {
				auto& prevChunk = mPageTable->GetChunk(task.prevPage0Pos.x, task.prevPage0Pos.y, task.prevMipLevel);
				prevChunk.SetInActive();
				prevChunk.node = nullptr;
			}
			if (task.nextMipLevel != -1 && task.nextPage0Pos != Math::Int2{ -1, -1 }) {
				auto& nextChunk = mPageTable->GetChunk(task.nextPage0Pos.x, task.nextPage0Pos.y, task.nextMipLevel);
				nextChunk.SetInLoading();
			}
			if (task.node != nullptr) {
				task.node->page0Pos = Math::Int2{ -1, -1 };
				task.node->mipLevel = -1;
			}

			// ����tilePos��Ӧ��ͼ��ռ��µ�tileRect
			const auto& tileSizeWithPadding = mRvtPhysicalTexture->GetTileSizeWithPadding();
			const Math::Int4 tileRectInImageSpace = Math::Int4{
				task.node->tilePos.x * (int32_t)tileSizeWithPadding,
				task.node->tilePos.y * (int32_t)tileSizeWithPadding,
				(int32_t)tileSizeWithPadding,
				(int32_t)tileSizeWithPadding
			};

			// ������
			int x = task.nextPage0Pos.x;
			int y = task.nextPage0Pos.y;
			int perSize = (int)std::pow(2, task.nextMipLevel);
			x = x - x % perSize;
			y = y - y % perSize;

			uint32_t paddingEffect = mRvtPhysicalTexture->GetPaddingSize() * perSize * (mCurrRvtRect.z / mTileCountPerAxis) / mTileSize;
			Math::Vector4 tileRectInWorldSpace = Math::Vector4{
				mCurrRvtRect.x + ((float)x / mTileCountPerAxis) * mCurrRvtRect.z - paddingEffect,
				mCurrRvtRect.y + ((float)y / mTileCountPerAxis) * mCurrRvtRect.w - paddingEffect,
				mCurrRvtRect.z * ((float)perSize / mTileCountPerAxis) + 2.0f * paddingEffect,
				mCurrRvtRect.w * ((float)perSize / mTileCountPerAxis) + 2.0f * paddingEffect
			};

			// �ؿ����
			const Math::Vector2 terrainMeterSize = mTerrainSystem->worldMeterSize;
			Math::Vector4 terrainRectInWorldSpace = Math::Vector4{
				-(terrainMeterSize.x / 2.0f),
				-(terrainMeterSize.y / 2.0f),
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
			float scaleFactorWidth = tileRectInImageSpace.z / tileRectInWorldSpace.z;
			float scaleFactorHeight = tileRectInImageSpace.w / tileRectInWorldSpace.w;

			Math::Vector4 tileRectInImageSpaceOverlapped = Math::Vector4{
				tileRectInImageSpace.x + (tileRectInWorldSpaceOverlapped.x - tileRectInWorldSpace.x) * scaleFactorWidth,
				tileRectInImageSpace.y + (tileRectInWorldSpaceOverlapped.y - tileRectInWorldSpace.y) * scaleFactorHeight,
				tileRectInWorldSpaceOverlapped.z * scaleFactorWidth,
				tileRectInWorldSpaceOverlapped.w * scaleFactorHeight
			};

			// tileRect��terrainRect�ĳ���������ԭ�㣨���½ǣ�ƽ��
			Math::Vector4 scaleOffset = Math::Vector4{
				tileRectInWorldSpaceOverlapped.z / terrainRectInWorldSpace.z,
				tileRectInWorldSpaceOverlapped.w / terrainRectInWorldSpace.w,
				(tileRectInWorldSpaceOverlapped.x - terrainRectInWorldSpace.x) / terrainRectInWorldSpace.z,
				(tileRectInWorldSpaceOverlapped.y - terrainRectInWorldSpace.y) / terrainRectInWorldSpace.w
			};

			Math::Vector2 nowScale = Math::Vector2(terrainRectInWorldSpace.z / 15.0f,
				terrainRectInWorldSpace.w / 15.0f);
			Math::Vector4 tileOffset = Math::Vector4(nowScale.x * scaleOffset.x,
				nowScale.y * scaleOffset.y, scaleOffset.z * nowScale.x, scaleOffset.w * nowScale.y);

			// �����Ӿֲ��ռ䵽ͼ��ռ��MVP����
			float physicalTextureSize = (float)mRvtPhysicalTexture->GetPhysicalTextureSize();

			// ��������
			float l = tileRectInImageSpaceOverlapped.x * 2.0f / physicalTextureSize - 1.0f;
			float r = (tileRectInImageSpaceOverlapped.x + tileRectInImageSpaceOverlapped.z) * 2.0f / physicalTextureSize - 1.0f;
			float b = tileRectInImageSpaceOverlapped.y * 2.0f / physicalTextureSize - 1.0f;
			float t = (tileRectInImageSpaceOverlapped.y + tileRectInImageSpaceOverlapped.w) * 2.0f / physicalTextureSize - 1.0f;

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

			gpuDrawRequests.emplace_back(tileRectInWorldSpaceOverlapped, tileRectInImageSpaceOverlapped, mvpMatrix, scaleOffset, tileOffset);
			mLoadingTasks.at(mRvtFrameTracker->GetCurrFrameIndex()).emplace_back(task);

			count--;
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
			commandBuffer.PIXBeginEvent("UpdatePhysicalTexturePass");

			auto& physicalTextures = mRvtPhysicalTexture->GetPhysicalTextures();
			ASSERT_FORMAT(physicalTextures.size() > 0u, "PhysicalTextures Is Empty!");
			auto& rvtTileMapDesc = physicalTextures.at(0)->GetResourceFormat().GetTextureDesc();
			uint16_t width = static_cast<uint16_t>(rvtTileMapDesc.width);
			uint16_t height = static_cast<uint16_t>(rvtTileMapDesc.height);

			// Update Pass Data
			mUpdatePhysicalTexturePassData.drawRequestBufferIndex = mDrawPhysicalTextureRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.terrainHeightMapIndex = mTerrainSystem->terrainHeightMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.terrainNormalMapIndex = mTerrainSystem->terrainNormalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.terrainSplatMapIndex = mTerrainSystem->terrainSplatMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.terrainMeterSize = mTerrainSystem->worldMeterSize;
			mUpdatePhysicalTexturePassData.terrainHeightScale = mTerrainSystem->worldHeightScale;

			mUpdatePhysicalTexturePassData.rChannelAlbedoMapIndex = mTerrainSystem->grassAlbedoMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.rChannelNormalMapIndex = mTerrainSystem->grassNormalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.rChannelHeightMapIndex = mTerrainSystem->grassHeightMap->GetSRDescriptor()->GetHeapIndex();
			// mUpdatePhysicalTexturePassData.rChannelRoughnessMapIndex = mTerrainSystem->grassRoughnessMap->GetSRDescriptor()->GetHeapIndex();

			mUpdatePhysicalTexturePassData.gChannelAlbedoMapIndex = mTerrainSystem->mudAlbedoMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.gChannelNormalMapIndex = mTerrainSystem->mudNormalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.gChannelHeightMapIndex = mTerrainSystem->mudHeightMap->GetSRDescriptor()->GetHeapIndex();
			// mUpdatePhysicalTexturePassData.gChannelRoughnessMapIndex = mTerrainSystem->mudRoughnessMap->GetSRDescriptor()->GetHeapIndex();

			mUpdatePhysicalTexturePassData.bChannelAlbedoMapIndex = mTerrainSystem->cliffAlbedoMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.bChannelNormalMapIndex = mTerrainSystem->cliffNormalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.bChannelHeightMapIndex = mTerrainSystem->cliffHeightMap->GetSRDescriptor()->GetHeapIndex();
			// mUpdatePhysicalTexturePassData.bChannelRoughnessMapIndex = mTerrainSystem->cliffRoughnessMap->GetSRDescriptor()->GetHeapIndex();

			mUpdatePhysicalTexturePassData.aChannelAlbedoMapIndex = mTerrainSystem->snowAlbedoMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.aChannelNormalMapIndex = mTerrainSystem->snowNormalMap->GetSRDescriptor()->GetHeapIndex();
			mUpdatePhysicalTexturePassData.aChannelHeightMapIndex = mTerrainSystem->snowHeightMap->GetSRDescriptor()->GetHeapIndex();
			// mUpdatePhysicalTexturePassData.aChannelRoughnessMapIndex = mTerrainSystem->snowRoughness->GetSRDescriptor()->GetHeapIndex();

			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdatePhysicalTexturePassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdatePhysicalTexturePassData, sizeof(UpdatePhysicalTexturePassData));

			// �����������ϴ���GPU��ȥ
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mDrawPhysicalTextureRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mDrawPhysicalTextureRequestBuffer, 0u, gpuDrawRequests.data(), gpuDrawRequests.size() * sizeof(GPUDrawPhysicalTextureRequest));

			// ����LookUpMap
			barrierBatch = commandBuffer.TransitionImmediately(mDrawPhysicalTextureRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += mMainResourceStateTracker->TransitionImmediately(physicalTextures.at(0), GHL::EResourceState::RenderTarget);
			barrierBatch += mMainResourceStateTracker->TransitionImmediately(physicalTextures.at(1), GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);

			commandBuffer.SetRenderTargets({ physicalTextures.at(0), physicalTextures.at(1) });
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState("UpdatePhysicalTexture");
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

	// ����ҳ������
	void RuntimeVirtualTextureSystem::UpdatePageTableTexturePass() {

		// ÿ�θ��¶�ֻ���µ�ǰPhysicalTexture�д��ڵ�Tile
		auto& tileCache = mRvtPhysicalTexture->GetTileCache();
		auto* currNode = tileCache.GetHead();
		auto* currTail = tileCache.GetTail();

		std::vector<GPUDrawPageTableTextureRequest> gpuDrawRequests;
		while (currNode != nullptr) {
			// ����Node�Ƿ���PageTablePos��Ӧ
			if (currNode->page0Pos != Math::Int2{ -1, -1 } && currNode->mipLevel != -1) {
				// ����PageTablePos��Ӧ����������������ȥ
				auto& chunk = mPageTable->GetChunk(currNode->page0Pos.x, currNode->page0Pos.y, currNode->mipLevel);
				gpuDrawRequests.emplace_back(
					Math::Vector4{ (float)chunk.rect.x, (float)chunk.rect.y, (float)chunk.rect.z, (float)chunk.rect.w },
					chunk.mipLevel,
					Math::Vector2{ (float)chunk.node->tilePos.x, (float)chunk.node->tilePos.y }
				);
			}

			currNode = currNode->next;
		}

		if (gpuDrawRequests.empty()) {
			return;
		}

		// ��gpuDrawRequests��������ֵ�����mipLevel����ǰ�档ʹ��ֵ��С��mipLevel�ڻ���ʱ���Ը���ֵ�����mipLevel
		std::sort(gpuDrawRequests.begin(), gpuDrawRequests.end(),
			[](const GPUDrawPageTableTextureRequest& requestA, const GPUDrawPageTableTextureRequest& requestB) {
				return requestA.mipLevel > requestB.mipLevel;
			});

		// ����Matrix
		for (int i = 0; i < gpuDrawRequests.size(); i++) {
			// ת����PageTableTexture��ImageSpace
			float size = gpuDrawRequests[i].rect.z / mTileCountPerAxis;
			gpuDrawRequests[i].mvpMatrix = Math::Matrix4{
				Math::Vector3{ gpuDrawRequests[i].rect.x / mTileCountPerAxis, gpuDrawRequests[i].rect.y / mTileCountPerAxis, 0.0f },
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
			commandBuffer.PIXBeginEvent("UpdatePageTableTexturePass");

			auto& pageTableTextureDesc = mPageTableTexture->GetResourceFormat().GetTextureDesc();
			uint16_t width = static_cast<uint16_t>(pageTableTextureDesc.width);
			uint16_t height = static_cast<uint16_t>(pageTableTextureDesc.height);

			// Update Pass Data
			mUpdatePageTableTexturePassData.drawRequestBufferIndex = mDrawPageTableTextureRequestBuffer->GetSRDescriptor()->GetHeapIndex();
			auto passDataAlloc = mRvtLinearBufferAllocator->Allocate(sizeof(UpdatePageTableTexturePassData));
			memcpy(passDataAlloc.cpuAddress, &mUpdatePageTableTexturePassData, sizeof(UpdatePageTableTexturePassData));

			// �����������ϴ���GPU��ȥ
			auto barrierBatch = GHL::ResourceBarrierBatch{};
			barrierBatch = commandBuffer.TransitionImmediately(mDrawPageTableTextureRequestBuffer, GHL::EResourceState::CopyDestination);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.UploadBufferRegion(mDrawPageTableTextureRequestBuffer, 0u, gpuDrawRequests.data(), gpuDrawRequests.size() * sizeof(GPUDrawPageTableTextureRequest));

			// ����LookUpMap
			barrierBatch = commandBuffer.TransitionImmediately(mDrawPageTableTextureRequestBuffer, GHL::EResourceState::NonPixelShaderAccess);
			barrierBatch += mMainResourceStateTracker->TransitionImmediately(mPageTableTexture, GHL::EResourceState::RenderTarget);
			commandBuffer.FlushResourceBarrier(barrierBatch);
			commandBuffer.ClearRenderTarget(mPageTableTexture);
			commandBuffer.SetRenderTarget(mPageTableTexture);
			commandBuffer.SetViewport(GHL::Viewport{ 0u, 0u, width, height });
			commandBuffer.SetScissorRect(GHL::Rect{ 0u, 0u, width, height });
			commandBuffer.SetGraphicsRootSignature();
			commandBuffer.SetGraphicsPipelineState("UpdatePageTableTexture");
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

	void RuntimeVirtualTextureSystem::ClearTaskQueue() {

		for (auto& task : mTasks) {
			if (task.prevMipLevel != -1 && task.prevPage0Pos != Math::Int2{ -1, -1 }) {
				// do nothing
			}
			if (task.nextMipLevel != -1 && task.nextPage0Pos != Math::Int2{ -1, -1 }) {
				auto& nextChunk = mPageTable->GetChunk(task.nextPage0Pos.x, task.nextPage0Pos.y, task.nextMipLevel);
				nextChunk.SetInActive();
			}

			if (task.node != nullptr) {
				auto& tileCache = mRvtPhysicalTexture->GetTileCache();
				tileCache.AddHead(task.node);
			}
		}
		mTasks.clear();

	}

	void RuntimeVirtualTextureSystem::DistributeTile(int page0PosX, int page0PosY, int mipLevel) {
		if (mipLevel > mMaxMipLevel || mipLevel < 0 || 
			page0PosX < 0 || page0PosY < 0 ||
			page0PosX >= mTileCountPerAxis || page0PosY >= mTileCountPerAxis) {
			return;
		}

		auto& chunk = mPageTable->GetChunk(page0PosX, page0PosY, mipLevel);
		auto& tileCache = mRvtPhysicalTexture->GetTileCache();
		// Use inQueue and inTexture to test whether need to enqueue the task
		if (chunk.inQueue || chunk.inLoading) {
			// do nothing
		}
		else if (chunk.inTexture) {
			// set active
			tileCache.Remove(chunk.node);
			tileCache.AddTail(chunk.node);
		}
		else {
			// push task
			mTasks.emplace_back(Math::Int2{ page0PosX, page0PosY }, mipLevel);
			// update chunk
			chunk.SetInQueue();
		}

	}

	void RuntimeVirtualTextureSystem::OnRvtFrameCompleted(uint8_t rvtFrameIndex) {

		// ��ȡ��֡��������ɵ�����
		auto& completedTasks = mLoadingTasks.at(rvtFrameIndex);

		if (completedTasks.empty()) {
			return;
		}

		// ����TileCache
		for (auto& task : completedTasks) {
			auto& chunk = mPageTable->GetChunk(task.nextPage0Pos.x, task.nextPage0Pos.y, task.nextMipLevel);
			chunk.SetInTexture();
			chunk.node = task.node;

			if (task.node != nullptr) {
				task.node->page0Pos = task.nextPage0Pos;
				task.node->mipLevel = task.nextMipLevel;
				auto& tileCache = mRvtPhysicalTexture->GetTileCache();
				tileCache.AddTail(task.node);
			}
		}

		completedTasks.clear();
	}

	void RuntimeVirtualTextureSystem::InitializeGraphicsObject(TerrainSystem* terrainSystem) {
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
			mRvtGrahpicsQueue->SetDebugName("RuntimeVirtualTextureSystem");
			mRvtFrameFence = std::make_unique<GHL::Fence>(device);

			mRvtFrameTracker = std::make_unique<Renderer::RingFrameTracker>(mMaxRvtFrameCount);
			mRvtResourceStateTracker = std::make_unique<Renderer::ResourceStateTracker>();
			mRvtLinearBufferAllocator = std::make_unique<Renderer::LinearBufferAllocator>(device, mRvtFrameTracker.get());
			mRvtPoolCommandListAllocator = std::make_unique<Renderer::PoolCommandListAllocator>(device, mRvtFrameTracker.get());

			mRvtFrameTracker->AddFrameCompletedCallBack(
				[this](const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue) {
					this->OnRvtFrameCompleted(attribute.frameIndex);
				}
			);
		}

		// ����QuadMesh
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

		// ����Shader
		mMainShaderManger->CreateGraphicsShader("UpdatePageTableTexture",
			[](GraphicsStateProxy& proxy) {
				proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/UpdateRvtLookUpMap.hlsl";
				proxy.psFilepath = proxy.vsFilepath;
				proxy.depthStencilDesc.DepthEnable = false;
				proxy.renderTargetFormatArray = {
					DXGI_FORMAT_R16G16B16A16_FLOAT	// PageTableTexture
				};
			}
		);

		mMainShaderManger->CreateGraphicsShader("UpdatePhysicalTexture",
			[](GraphicsStateProxy& proxy) {
				proxy.vsFilepath = "E:/MyProject/DXDance/Resources/Shaders/Engine/GPUDrivenTerrain/UpdateRvtTiledMap.hlsl";
				proxy.psFilepath = proxy.vsFilepath;
				proxy.depthStencilDesc.DepthEnable = false;
				proxy.renderTargetFormatArray = {
					DXGI_FORMAT_R16G16B16A16_FLOAT,	// PhysicalTextureAlbedo
					DXGI_FORMAT_R16G16B16A16_FLOAT	// PhysicalTextureNormal
				};
			}
		);

		// ����RequestBuffer
		BufferDesc _DrawPhysicalTextureRequestBufferDesc{};
		_DrawPhysicalTextureRequestBufferDesc.stride = sizeof(GPUDrawPhysicalTextureRequest);
		_DrawPhysicalTextureRequestBufferDesc.size = _DrawPhysicalTextureRequestBufferDesc.stride * mTileCount;
		_DrawPhysicalTextureRequestBufferDesc.usage = GHL::EResourceUsage::Default;
		_DrawPhysicalTextureRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_DrawPhysicalTextureRequestBufferDesc.initialState = GHL::EResourceState::Common;
		_DrawPhysicalTextureRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
		mDrawPhysicalTextureRequestBuffer = resourceAllocator->Allocate(device, _DrawPhysicalTextureRequestBufferDesc, descriptorAllocator, nullptr);
		mDrawPhysicalTextureRequestBuffer->SetDebugName("DrawPhysicalTextureRequestBuffer");

		mRvtResourceStateTracker->StartTracking(mDrawPhysicalTextureRequestBuffer);

		BufferDesc _DrawPageTableTextureRequestBufferDesc{};
		_DrawPageTableTextureRequestBufferDesc.stride = sizeof(GPUDrawPageTableTextureRequest);
		_DrawPageTableTextureRequestBufferDesc.size = _DrawPageTableTextureRequestBufferDesc.stride * mRvtPhysicalTexture->GetTileCount();
		_DrawPageTableTextureRequestBufferDesc.usage = GHL::EResourceUsage::Default;
		_DrawPageTableTextureRequestBufferDesc.miscFlag = GHL::EBufferMiscFlag::StructuredBuffer;
		_DrawPageTableTextureRequestBufferDesc.initialState = GHL::EResourceState::Common;
		_DrawPageTableTextureRequestBufferDesc.expectedState = GHL::EResourceState::CopyDestination | GHL::EResourceState::NonPixelShaderAccess;
		mDrawPageTableTextureRequestBuffer = resourceAllocator->Allocate(device, _DrawPageTableTextureRequestBufferDesc, descriptorAllocator, nullptr);
		mDrawPageTableTextureRequestBuffer->SetDebugName("DrawPageTableTextureRequestBuffer");

		mRvtResourceStateTracker->StartTracking(mDrawPageTableTextureRequestBuffer);

		// ����PageTableTexture
		TextureDesc _PageTableTextureDesc{};
		_PageTableTextureDesc.width = mTileCountPerAxis;
		_PageTableTextureDesc.height = mTileCountPerAxis;
		_PageTableTextureDesc.format = DXGI_FORMAT_R16G16B16A16_FLOAT;
		_PageTableTextureDesc.expectedState = GHL::EResourceState::RenderTarget | GHL::EResourceState::PixelShaderAccess;
		_PageTableTextureDesc.clearVaule = GHL::ColorClearValue{ 0.0f, 0.0f, 0.0f, 0.0f };
		mPageTableTexture = resourceAllocator->Allocate(device, _PageTableTextureDesc, descriptorAllocator, nullptr);
		mPageTableTexture->SetDebugName("PageTableTexture");

		renderGraph->ImportResource("PageTableTexture", mPageTableTexture);	// ����Ⱦ�߳��е�Pass��Ҫ���ʸ���Դ
		resourceStateTracker->StartTracking(mPageTableTexture);
	}

}