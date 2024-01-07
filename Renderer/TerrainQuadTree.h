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

		ResourceResidencyState resourceResidencyState;	// �Ĳ����ڵ��Ӧ��Դ��פ��״̬
		uint8_t isBranch;		// �ڵ��Ƿ���չ
		uint8_t isReference;	// �ڵ�����Ƿ�ʹ��
		uint8_t pad1;

		// �Ĳ����ڵ�����߶Ⱥ���С�߶�
		float minHeight;
		float maxHeight;

		// 16Bytes
	};

	struct TerrainQuadLODDescriptor {
	public:
		uint32_t nodeMeterSize;		// ��LOD��ÿһ��Node�ı߳�(��)(Node��������)
		uint32_t nodeStartOffset;	// ��LOD�еĵ�һ��Node�Ŀ�ʼƫ����
		uint32_t nodeCount;			// ��LOD�е�Node���ܸ���
		
		float pad1;
	};


	class TerrainQuadNode {
	public:
		TerrainQuadNode() = default;
		~TerrainQuadNode() = default;

		TerrainQuadNode(TerrainQuadTree* terrainQuadTree, uint8_t nodeLocationX, uint8_t nodeLocationY, uint8_t nodeLOD);

		// ��ȡ�Ĳ����ڵ��LOD����
		uint32_t GetLODDescriptorIndex() const;

		// ��ȡ�Ĳ����ڵ���ȫ�ڵ��������е�����
		uint32_t GetNodeDescriptorIndex() const;

		// �����Ĳ����ڵ�����λ�õ�����
		Math::Vector3 GetWsPosition() const;

		inline const auto& GetNodeID() const { return mNodeID; }

	private:
		TerrainQuadTree* mTerrainQuadTree{ nullptr };	// �����Ĳ���
		TerrainQuadNodeID mNodeID;
	};


	class TerrainQuadTree {
		friend class TerrainQuadNode;
		friend class TerrainQuadTreeBackend;
	public:
		TerrainQuadTree(TerrainRenderer* renderer);
		~TerrainQuadTree();

		// ��ʼ��
		void Initialize();

		// ÿ֡����
		void Update();

	private:
		// ��̨�߳�
		void BackendThread();

		// ����֮ǰ�ϴ������񣬸÷����ɺ�̨�̵߳���
		void ProcessUploadedTerrainQuadNodeQueue();

		// ����StreamIn/Out��QuadNode���÷����ɺ�̨�̵߳���
		void ProcessConsumedTerrainQuadNodeQueue();

		// ����Eviction���÷����ɺ�̨�̵߳���
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
		float		mMinLODNodeMeterSize{ 64.0f };		// LOD0��Ӧ�ĵؿ��СΪ64.0f
		float		mMaxLODNodeMeterSize{ 1024.0f };	// LOD4��Ӧ�ĵؿ��СΪ1024.0f
		uint32_t	mMaxLOD{ 4u };						// pow(2, 4) * 64 = 1024.0f���� LOD0 1 2 3 4
		std::vector<TerrainQuadLODDescriptor>  mTerrainQuadLODDescriptors;	// �Ĳ���ȫLOD����������
		std::vector<TerrainQuadNodeDescriptor> mTerrainQuadNodeDescriptors;	// �Ĳ���ȫ�ڵ�����������

		// �߳�ͬ������
		std::thread mThread;
		HANDLE mEvent;
		bool mThreadRunning{ true };

		struct ConsumedTerrainQuadNodeQueue {
		public:
			std::vector<TerrainQuadNode> streamInQuadNodes;		// ���̼߳�����ĵ�ǰ֡��Ҫ��Դ������Ĳ����ڵ�
			std::vector<TerrainQuadNode> streamOutQuadNodes;	// ���̼߳�����ĵ�ǰ֡��Ҫ��Դ�������Ĳ����ڵ�
			uint64_t renderFrameIndex{ 0u };					// ��ǰ��Ⱦ֡������
		};
		std::deque<ConsumedTerrainQuadNodeQueue> mConsumedTerrainQuadNodeQueuesDeque;	// ����Ⱦ���߳�д�룬��̨�̶߳�ȡ������
		std::mutex mConsumedDequeMutex;

		// �ӳ��޳���
		class EvictionDelay {
		public:
			EvictionDelay(uint32_t nFrames = 4u);
			~EvictionDelay();

			// ����ǰ֡��Ҫ�޳����Ĳ����ڵ��ۼ�����
			void MoveToNextFrame();
			void Clear();

			// ��������������Ҫ�޳����Ĳ����ڵ�
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