#pragma once
#include "Math/Vector.h"
#include "Renderer/TextureAtlas.h"
#include "GHL/Fence.h"

#include <vector>
#include <thread>
#include <semaphore>
#include <deque>
#include <mutex>

namespace Renderer {
	/*
	class RenderEngine;
	class TerrainRenderer;
	class TerrainQuadTree;

	struct TerrainQuadNodeID {
	public:
		uint8_t nodeLocationX;
		uint8_t nodeLocationY;
		uint8_t nodeLOD;
		uint8_t pad1;

	public:
		TerrainQuadNodeID() = default;
		~TerrainQuadNodeID() = default;
		TerrainQuadNodeID(uint8_t nodeLocationX, uint8_t nodeLocationY, uint8_t nodeLOD);
	};

	enum class ResourceResidencyState : uint8_t {
		NotResident = 0,
		Resident    = 1,
		Loading     = 2,
	};
	

	struct TerrainQuadNodeDescriptor {
	public:
		// 16Bytes
		TerrainQuadNodeID nodeID;

		ResourceResidencyState resourceResidencyState;	// 四叉树节点对应资源的驻留状态
		uint8_t isBranch;		// 节点是否被扩展
		uint8_t isReference;	// 节点最近是否被使用
		uint8_t pad1;

		// 四叉树节点的最大高度和最小高度
		float minHeight;
		float maxHeight;

		// 16Bytes
	};

	struct TerrainQuadLODDescriptor {
	public:
		uint32_t nodeMeterSize;		// 该LOD中每一个Node的边长(米)(Node是正方形)
		uint32_t nodeStartOffset;	// 该LOD中的第一个Node的开始偏移量
		uint32_t nodeCount;			// 该LOD中的Node的总个数
		
		float pad1;
	};


	class TerrainQuadNode {
	public:
		TerrainQuadNode() = default;
		~TerrainQuadNode() = default;

		TerrainQuadNode(TerrainQuadTree* terrainQuadTree, uint8_t nodeLocationX, uint8_t nodeLocationY, uint8_t nodeLOD);

		// 获取四叉树节点的LOD索引
		uint32_t GetLODDescriptorIndex() const;

		// 获取四叉树节点在全节点描述表中的索引
		uint32_t GetNodeDescriptorIndex() const;

		// 计算四叉树节点世界位置的坐标
		Math::Vector3 GetWsPosition() const;

		inline const auto& GetNodeID() const { return mNodeID; }

	private:
		TerrainQuadTree* mTerrainQuadTree{ nullptr };	// 地形四叉树
		TerrainQuadNodeID mNodeID;
	};


	class TerrainQuadTree {
		friend class TerrainQuadNode;
		friend class TerrainQuadTreeBackend;
	public:
		TerrainQuadTree(TerrainRenderer* renderer);
		~TerrainQuadTree();

		// 初始化
		void Initialize();

		// 每帧更新
		void Update();

	private:
		// 后台线程
		void BackendThread();

		// 处理之前上传的任务，该方法由后台线程调用
		void ProcessUploadedTerrainQuadNodeQueue();

		// 处理StreamIn/Out的QuadNode，该方法由后台线程调用
		void ProcessConsumedTerrainQuadNodeQueue();

		// 处理Eviction，该方法由后台线程调用
		void ProcessEvictionDelay();

	public:
		inline const auto  GetWorldMeterSize() const { return mWorldMeterSize; }
		inline const auto  GetHeightScale()    const { return mWorldHeightScale; }
		inline const auto& GetTerrainQuadLODDescriptors()  const { return mTerrainQuadLODDescriptors; }
		inline const auto& GetTerrainQuadNodeDescriptors() const { return mTerrainQuadNodeDescriptors; }

	private:
		TerrainRenderer* mRenderer{ nullptr };

		float		mNodeEvaluationC{ 1.2f };
		float		mWorldMeterSize{ 8192.0f };
		float		mWorldHeightScale{ 1325.0f };
		float		mMinLODNodeMeterSize{ 64.0f };		// LOD0对应的地块大小为64.0f
		float		mMaxLODNodeMeterSize{ 1024.0f };	// LOD4对应的地块大小为1024.0f
		uint32_t	mMaxLOD{ 4u };						// pow(2, 4) * 64 = 1024.0f，即 LOD0 1 2 3 4
		std::vector<TerrainQuadLODDescriptor>  mTerrainQuadLODDescriptors;	// 四叉树全LOD内容描述表
		std::vector<TerrainQuadNodeDescriptor> mTerrainQuadNodeDescriptors;	// 四叉树全节点内容描述表

		// 线程同步变量
		std::thread mThread;
		HANDLE mEvent;
		bool mThreadRunning{ true };

		struct ConsumedTerrainQuadNodeQueue {
		public:
			std::vector<TerrainQuadNode> streamInQuadNodes;		// 主线程计算出的当前帧需要资源流入的四叉树节点
			std::vector<TerrainQuadNode> streamOutQuadNodes;	// 主线程计算出的当前帧需要资源流出的四叉树节点
			uint64_t renderFrameIndex{ 0u };					// 当前渲染帧的索引
		};
		std::deque<ConsumedTerrainQuadNodeQueue> mConsumedTerrainQuadNodeQueuesDeque;	// 由渲染主线程写入，后台线程读取并处理
		std::mutex mConsumedDequeMutex;

		// 延迟剔除器
		class EvictionDelay {
		public:
			EvictionDelay(uint32_t nFrames = 4u);
			~EvictionDelay();

			// 将当前帧需要剔除的四叉树节点累加起来
			void MoveToNextFrame();
			void Clear();

			// 重新评估本来需要剔除的四叉树节点
			void Rescue(std::vector<TerrainQuadNodeDescriptor>& nodeDescriptors);

			inline void Append(const TerrainQuadNode& node) { mEvictionsBuffer[0].push_back(node); }
			inline void Append(const std::vector<TerrainQuadNode>& nodes) { mEvictionsBuffer[0].insert(mEvictionsBuffer[0].end(), nodes.begin(), nodes.end()); }
			inline std::vector<TerrainQuadNode>& GetReadyToEvict() { return mEvictionsBuffer.back(); }

		private:
			std::vector<std::vector<TerrainQuadNode>> mEvictionsBuffer;
		};
		EvictionDelay mEvictionDelay;

		// CopyQueue/CopyFence
		IDStorageQueue* mDStorageFileQueue = nullptr;
		GHL::Fence* mCopyFence = nullptr;

		struct UploadedTerrainQuadNodeQueue {
		public:
			std::vector<TerrainQuadNode> terrainQuadNodes;
			uint64_t copyFenceValue{ 0u };
		};
		std::queue<UploadedTerrainQuadNodeQueue> mUploadedTerrainQuadNodeQueues;
	};
	*/
}