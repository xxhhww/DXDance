#include "Renderer/TerrainQuadTree.h"
#include "Renderer/RenderEngine.h"

namespace Renderer {

	TerrainQuadNodeID::TerrainQuadNodeID(uint8_t _nodeLocationX, uint8_t _nodeLocationY, uint8_t _nodeLOD) 
	: nodeLocationX(_nodeLocationX)
	, nodeLocationY(_nodeLocationY)
	, nodeLOD(_nodeLOD) {}

	TerrainQuadNode::TerrainQuadNode(TerrainQuadTree* terrainQuadTree, uint8_t nodeLocationX, uint8_t nodeLocationY, uint8_t nodeLOD)
	: mTerrainQuadTree(terrainQuadTree) 
	, mNodeID(nodeLocationX, nodeLocationY, nodeLOD) {}

	// 获取四叉树节点的LOD索引
	uint32_t TerrainQuadNode::GetLODDescriptorIndex() const {
		return mNodeID.nodeLOD;
	}

	// 获取四叉树节点在全节点描述表中的索引
	uint32_t TerrainQuadNode::GetNodeDescriptorIndex() const {
		const auto& currLODDescriptor = mTerrainQuadTree->GetTerrainQuadLODDescriptors()[mNodeID.nodeLOD];
		uint32_t nodeCountPerRow = mTerrainQuadTree->GetWorldMeterSize() / currLODDescriptor.nodeMeterSize;
		return (currLODDescriptor.nodeStartOffset + mNodeID.nodeLocationY * nodeCountPerRow + mNodeID.nodeLocationX);
	}

	// 计算四叉树节点世界位置的坐标
	Math::Vector3 TerrainQuadNode::GetWsPosition() const {
		const auto& currLODDescriptor = mTerrainQuadTree->GetTerrainQuadLODDescriptors()[mNodeID.nodeLOD];
		const auto& currNodeDescriptor = mTerrainQuadTree->GetTerrainQuadNodeDescriptors()[GetNodeDescriptorIndex()];

		float nodeMeterSize = currLODDescriptor.nodeMeterSize;
		float nodeCountPerRow = mTerrainQuadTree->GetWorldMeterSize() / currLODDescriptor.nodeMeterSize;
		float nodeCountPerCol = mTerrainQuadTree->GetWorldMeterSize() / currLODDescriptor.nodeMeterSize;
		
		Math::Vector3 nodeWsPosition{ 0.0f, 0.0f, 0.0f };
		nodeWsPosition.x = (mNodeID.nodeLocationX - (nodeCountPerRow - 1.0f) * 0.5f) * nodeMeterSize;
		nodeWsPosition.y = (currNodeDescriptor.minHeight + currNodeDescriptor.maxHeight) * 0.5f;
		nodeWsPosition.z = (mNodeID.nodeLocationY - (nodeCountPerCol - 1.0f) * 0.5f) * nodeMeterSize;

		return nodeWsPosition;
	}

	void TerrainQuadTree::Initialize(RenderEngine* renderEngine) {
		mDStorageFileCopyQueue = renderEngine->mUploaderEngine->GetFileCopyQueue();
		mDStorageCopyFence = renderEngine->mUploaderEngine->GetCopyFence();
	}

	void TerrainQuadTree::AddPass(RenderEngine* renderEngine) {

	}

	void TerrainQuadTree::Update(RenderEngine* renderEngine) {
		const Math::Vector3 cameraPosition = renderEngine->mPipelineResourceStorage->rootConstantsPerFrame.currentRenderCamera.position;

		// 做四叉树Stream操作
		std::vector<TerrainQuadNode> streamInTerrainQuadNodeArray;
		std::vector<TerrainQuadNode> streamOutTerrainQuadNodeArray;

		std::queue<TerrainQuadNode> tempTerrainQuadNodeQueue;
		std::queue<TerrainQuadNode> tempTerrainQuadNodeQueueForStreamOut;

		// 初始化tempTerrainQuadNodeQueue
		{
			uint32_t nodeCountPerAxisInMaxLOD = mWorldMeterSize / mMaxLODNodeMeterSize;
			for (uint32_t i = 0; i < nodeCountPerAxisInMaxLOD; i++) {
				for (uint32_t j = 0; j < nodeCountPerAxisInMaxLOD; j++) {
					tempTerrainQuadNodeQueue.emplace(this, i, j, mMaxLOD);
				}
			}
		}

		// 执行四叉树分割操作
		while (!tempTerrainQuadNodeQueue.empty()) {
			auto currQuadNode = tempTerrainQuadNodeQueue.front();
			tempTerrainQuadNodeQueue.pop();

			const auto& currNodeID = currQuadNode.GetNodeID();
			auto& currNodeDescriptor = mTerrainQuadNodeDescriptors[currQuadNode.GetNodeDescriptorIndex()];
			auto& currLODDescriptor = mTerrainQuadLODDescriptors[currQuadNode.GetLODDescriptorIndex()];

			// 该节点被需要，因此填充进streamIn
			streamInTerrainQuadNodeArray.emplace_back(currQuadNode);

			// 如果已经是最小LOD的节点，后续不需要再执行了
			if (currNodeID.nodeLOD == 0) {
				continue;
			}

			// 计算摄像机与四叉树节点的距离
			Math::Vector3 currNodeWsPosition = currQuadNode.GetWsPosition();
			float distance = (cameraPosition - currNodeWsPosition).Length();
			float factor = distance / (currLODDescriptor.nodeMeterSize * mNodeEvaluationC);

			if (factor < 1.0f) {
				// 执行分割操作
				tempTerrainQuadNodeQueue.emplace(this, currNodeID.nodeLocationX * 2,     currNodeID.nodeLocationY * 2,     currNodeID.nodeLOD - 1);
				tempTerrainQuadNodeQueue.emplace(this, currNodeID.nodeLocationX * 2 + 1, currNodeID.nodeLocationY * 2,     currNodeID.nodeLOD - 1);
				tempTerrainQuadNodeQueue.emplace(this, currNodeID.nodeLocationX * 2,     currNodeID.nodeLocationY * 2 + 1, currNodeID.nodeLOD - 1);
				tempTerrainQuadNodeQueue.emplace(this, currNodeID.nodeLocationX * 2 + 1, currNodeID.nodeLocationY * 2 + 1, currNodeID.nodeLOD - 1);
				currNodeDescriptor.isBranch = true;
			}
			else {
				// 不执行分割
				// 如果该节点当前帧不执行分割，但是在上一帧却执行分割，则需要将子节点加入队列做StreamOut判断
				if (currNodeDescriptor.isBranch == true) {
					tempTerrainQuadNodeQueueForStreamOut.emplace(this, currNodeID.nodeLocationX * 2,     currNodeID.nodeLocationY * 2,     currNodeID.nodeLOD - 1);
					tempTerrainQuadNodeQueueForStreamOut.emplace(this, currNodeID.nodeLocationX * 2 + 1, currNodeID.nodeLocationY * 2,     currNodeID.nodeLOD - 1);
					tempTerrainQuadNodeQueueForStreamOut.emplace(this, currNodeID.nodeLocationX * 2,     currNodeID.nodeLocationY * 2 + 1, currNodeID.nodeLOD - 1);
					tempTerrainQuadNodeQueueForStreamOut.emplace(this, currNodeID.nodeLocationX * 2 + 1, currNodeID.nodeLocationY * 2 + 1, currNodeID.nodeLOD - 1);
					currNodeDescriptor.isBranch = false;
				}
			}
		}

		// 判断真正的StreamOut
		while (!tempTerrainQuadNodeQueueForStreamOut.empty()) {
			auto currQuadNode = tempTerrainQuadNodeQueueForStreamOut.front();
			tempTerrainQuadNodeQueueForStreamOut.pop();

			const auto& currNodeID = currQuadNode.GetNodeID();
			auto& currNodeDescriptor = mTerrainQuadNodeDescriptors[currQuadNode.GetNodeDescriptorIndex()];
			const auto& currLODDescriptor = mTerrainQuadLODDescriptors[currQuadNode.GetLODDescriptorIndex()];

			streamOutTerrainQuadNodeArray.emplace_back(currQuadNode);

			// 如果已经是最小LOD的节点，后续不需要再执行了
			if (currNodeID.nodeLOD == 0) {
				continue;
			}

			if (currNodeDescriptor.isBranch == true) {
				// 理论来说，不会进入此判断内部
				tempTerrainQuadNodeQueueForStreamOut.emplace(this, currNodeID.nodeLocationX * 2,     currNodeID.nodeLocationY * 2,     currNodeID.nodeLOD - 1);
				tempTerrainQuadNodeQueueForStreamOut.emplace(this, currNodeID.nodeLocationX * 2 + 1, currNodeID.nodeLocationY * 2,     currNodeID.nodeLOD - 1);
				tempTerrainQuadNodeQueueForStreamOut.emplace(this, currNodeID.nodeLocationX * 2,     currNodeID.nodeLocationY * 2 + 1, currNodeID.nodeLOD - 1);
				tempTerrainQuadNodeQueueForStreamOut.emplace(this, currNodeID.nodeLocationX * 2 + 1, currNodeID.nodeLocationY * 2 + 1, currNodeID.nodeLOD - 1);
				currNodeDescriptor.isBranch = false;
			}
		}

		RingFrameTracker* frameTracker = renderEngine->mFrameTracker.get();
		const auto& currFrameAttribute = frameTracker->GetCurrFrameAttribute();
		ConsumedTerrainQuadNodeQueue consumedTerrainQuadNodeQueue;
		consumedTerrainQuadNodeQueue.streamInQuadNodes = streamInTerrainQuadNodeArray;
		consumedTerrainQuadNodeQueue.streamOutQuadNodes = streamOutTerrainQuadNodeArray;
		consumedTerrainQuadNodeQueue.renderFrameIndex = currFrameAttribute.frameIndex;

		{
			std::lock_guard lock(mConsumedDequeMutex);
			mConsumedTerrainQuadNodeQueuesDeque.emplace_back(std::move(consumedTerrainQuadNodeQueue));
		}

		// 通知BackendThread线程
		SetEvent(mEvent);
	}

	void TerrainQuadTree::BackendThread() {
		while (mThreadRunning) {
			// 当前没有任何任务
			if (mUploadedTerrainQuadNodeQueues.empty() && mConsumedTerrainQuadNodeQueuesDeque.empty()) {
				WaitForSingleObject(mEvent, INFINITE);
			}

			// 被唤醒，则说明有需要消费的TerrainQuadNodeQueue
			while (!mUploadedTerrainQuadNodeQueues.empty() || !mConsumedTerrainQuadNodeQueuesDeque.empty()) {
				// 在唤醒状态下，只要上述两个队列不空，则轮询处理(对ConsumedTerrain...是否为空的判断可能会因为多线程而导致一些问题)

				ProcessConsumedTerrainQuadNodeQueue();

				ProcessEvictionDelay();

				ProcessUploadedTerrainQuadNodeQueue();
			}
		}
	}

	void TerrainQuadTree::ProcessUploadedTerrainQuadNodeQueue() {
		uint64_t completedCopyFenceValue = mDStorageCopyFence->CompletedValue();
		while (!mUploadedTerrainQuadNodeQueues.empty() && mUploadedTerrainQuadNodeQueues.front().copyFenceValue <= completedCopyFenceValue) {
			const auto uploadedTerrainQuadNodeQueue = mUploadedTerrainQuadNodeQueues.front();
			mUploadedTerrainQuadNodeQueues.pop();

			for (const auto& quadNode : uploadedTerrainQuadNodeQueue.terrainQuadNodes) {
				auto& currNodeDescriptor = mTerrainQuadNodeDescriptors[quadNode.GetNodeDescriptorIndex()];
				const auto& currLODDescriptor = mTerrainQuadLODDescriptors[quadNode.GetLODDescriptorIndex()];

				currNodeDescriptor.resourceResidencyState = ResourceResidencyState::Resident;
			}
		}
	}

	// 处理StreamIn/Out的QuadNode，该方法由后台线程调用
	void TerrainQuadTree::ProcessConsumedTerrainQuadNodeQueue() {
		mEvictionDelay.MoveToNextFrame();

		// 寻找最新鲜的QuadNodeQueue
		bool queueFound = false;
		ConsumedTerrainQuadNodeQueue consumedTerrainQuadNodeQueue;
		{
			std::lock_guard lock(mConsumedDequeMutex);
			if (mConsumedTerrainQuadNodeQueuesDeque.empty()) {
				queueFound = false;
			}
			else {
				auto& tagConsumedTerrainQuadNodeQueue = mConsumedTerrainQuadNodeQueuesDeque.back();
				std::swap(consumedTerrainQuadNodeQueue.streamInQuadNodes, tagConsumedTerrainQuadNodeQueue.streamInQuadNodes);
				std::swap(consumedTerrainQuadNodeQueue.streamOutQuadNodes, tagConsumedTerrainQuadNodeQueue.streamOutQuadNodes);
				consumedTerrainQuadNodeQueue.renderFrameIndex = tagConsumedTerrainQuadNodeQueue.renderFrameIndex;
				mConsumedTerrainQuadNodeQueuesDeque.clear();
				queueFound = true;
			}
		}
		if (!queueFound) return;

		const auto& streamInTerrainQuadNodeQueue = consumedTerrainQuadNodeQueue.streamInQuadNodes;
		const auto& streamOutTerrainQuadNodeQueue = consumedTerrainQuadNodeQueue.streamOutQuadNodes;
		const auto& renderFrameIndex = consumedTerrainQuadNodeQueue.renderFrameIndex;
		
		mUploadedTerrainQuadNodeQueues.emplace();
		auto& uploadedTerrainQuadNodeQueue = mUploadedTerrainQuadNodeQueues.back();

		for (const auto& quadNode : streamOutTerrainQuadNodeQueue) {
			auto& currNodeDescriptor = mTerrainQuadNodeDescriptors[quadNode.GetNodeDescriptorIndex()];
			const auto& currLODDescriptor = mTerrainQuadLODDescriptors[quadNode.GetLODDescriptorIndex()];

			currNodeDescriptor.isReference = false;
		}

		// 处理所有的StreamInTerrainQuadNode
		for (auto& quadNode : streamInTerrainQuadNodeQueue) {
			auto& currNodeDescriptor = mTerrainQuadNodeDescriptors[quadNode.GetNodeDescriptorIndex()];
			const auto& currLODDescriptor = mTerrainQuadLODDescriptors[quadNode.GetLODDescriptorIndex()];

			currNodeDescriptor.isReference = true;
			// 资源正在加载或者已经驻留，则跳过
			if (currNodeDescriptor.resourceResidencyState != ResourceResidencyState::NotResident) {
				continue;
			}

			for (auto& textureAtlas : mTextureAtlasArray) {
				const auto& textureAtlasFileDescriptor = textureAtlas->GetTextureAtlasFileDescriptor();
				const auto& subresourceOffset = textureAtlasFileDescriptor->GetSubresourceOffset(quadNode.GetNodeDescriptorIndex());
				const auto& subresourceFormat = textureAtlas->GetSubresourceFormat();

				// 首先在默认显存堆上使用Placed方式创建图集子资源
				TextureAtlasTile* textureAtlasTile = textureAtlas->GetSubresource(quadNode.GetNodeDescriptorIndex());
				textureAtlasTile->Create();

				// 填充DSTORAGE_REQUEST
				DSTORAGE_REQUEST dsRequest{};
				dsRequest.Options.SourceType = DSTORAGE_REQUEST_SOURCE_FILE;
				dsRequest.Options.DestinationType = DSTORAGE_REQUEST_DESTINATION_TEXTURE_REGION;
				dsRequest.Options.CompressionFormat = (DSTORAGE_COMPRESSION_FORMAT)textureAtlasFileDescriptor->GetCompressionFormat();
				dsRequest.Destination.Texture.Resource = textureAtlasTile->D3DResource();
				dsRequest.Destination.Texture.SubresourceIndex = 0u;
				dsRequest.UncompressedSize = subresourceFormat.GetSizeInBytes();
				dsRequest.Source.File.Source = textureAtlas->GetDSFileHandle();
				dsRequest.Source.File.Offset = subresourceOffset.offset;
				dsRequest.Source.File.Size = subresourceOffset.byteNums;

				mDStorageFileCopyQueue->EnqueueRequest(&dsRequest);
			}
			currNodeDescriptor.resourceResidencyState = ResourceResidencyState::Loading;

			uploadedTerrainQuadNodeQueue.terrainQuadNodes.emplace_back(std::move(quadNode));
		}
		uploadedTerrainQuadNodeQueue.copyFenceValue = mDStorageCopyFence->IncrementExpectedValue();
		mDStorageFileCopyQueue->EnqueueSignal(mDStorageCopyFence->D3DFence(), mDStorageCopyFence->ExpectedValue());
		mDStorageFileCopyQueue->Submit();
		// mDStorageCopyFence->Wait();
		
		mEvictionDelay.Append(streamOutTerrainQuadNodeQueue);
		mEvictionDelay.Rescue(mTerrainQuadNodeDescriptors);
	}

	void TerrainQuadTree::ProcessEvictionDelay() {
		if (mEvictionDelay.GetReadyToEvict().empty()) {
			return;
		}

		auto& readyEvictions = mEvictionDelay.GetReadyToEvict();
		uint32_t numDelayed = 0u;
		for (auto& quadNode : readyEvictions) {
			auto& currNodeDescriptor = mTerrainQuadNodeDescriptors[quadNode.GetNodeDescriptorIndex()];
			const auto& currLODDescriptor = mTerrainQuadLODDescriptors[quadNode.GetLODDescriptorIndex()];

			// 资源驻留
			if (currNodeDescriptor.resourceResidencyState == ResourceResidencyState::Resident) {
				for (auto& textureAtlas : mTextureAtlasArray) {
					// 获得节点对应的Tile(子资源)
					TextureAtlasTile* textureAtlasTile = textureAtlas->GetSubresource(quadNode.GetNodeDescriptorIndex());
					// 释放该节点对资源的占据状态(并不是直接释放显存资源)
					textureAtlasTile->Release();
				}
				currNodeDescriptor.resourceResidencyState = ResourceResidencyState::NotResident;
			}
			// 资源仍在加载
			else if (currNodeDescriptor.resourceResidencyState == ResourceResidencyState::Loading) {
				// 保留该QuadNode下次再来检测
				readyEvictions[numDelayed] = quadNode;
				numDelayed++;
			}
		}

		readyEvictions.resize(numDelayed);
	}

	TerrainQuadTree::EvictionDelay::EvictionDelay(uint32_t nFrames) {
		mEvictionsBuffer.resize(nFrames);
	}

	TerrainQuadTree::EvictionDelay::~EvictionDelay() {}

	void TerrainQuadTree::EvictionDelay::MoveToNextFrame() {
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

	void TerrainQuadTree::EvictionDelay::Clear() {
		for (auto& buffer : mEvictionsBuffer) {
			buffer.clear();
		}
	}

	void TerrainQuadTree::EvictionDelay::Rescue(std::vector<TerrainQuadNodeDescriptor>& nodeDescriptors) {
		// note: it is possible even for the most recent evictions to have refcount > 0
		// because a tile can be evicted then loaded again within a single ProcessFeedback() call
		for (auto& evictions : mEvictionsBuffer) {
			uint32_t numPending = (uint32_t)evictions.size();
			for (uint32_t i = 0; i < numPending;) {
				auto& quadNode = evictions[i];
				auto& currNodeDescriptor = nodeDescriptors[quadNode.GetNodeDescriptorIndex()];
				// on rescue, swap a later tile in and re-try the check
				// this re-orders the queue, but we can tolerate that
				// because the residency map is built bottom-up
				if (currNodeDescriptor.isReference == true) {
					numPending--;
					quadNode = evictions[numPending];
				}
				else { // refcount still 0, this tile may still be evicted 
					i++;
				}
			}
			evictions.resize(numPending);
		}
	}

}