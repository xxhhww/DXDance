#pragma once
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceStateTracker.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"

#include "Renderer/TerrainTextureAtlasTileCache.h"

#include "GHL/DirectStorageQueue.h"


namespace Renderer {

	/*
	* ������Դ��������
	*/
	struct TerrainNodeRequestTask {
	public:
		TerrainTextureAtlasTileCache::Node* atlasNode{ nullptr };	// һ��ͼ��Ԫ��
		int32_t prevTerrainNodeIndex{ -1 };		// ǰ�ε��νڵ�����
		int32_t nextTerrainNodeIndex{ -1 };		// ���ε��νڵ�����
	};

	/*
	* ������θ��µ��νڵ������Ľṹ��
	*/
	struct GpuUpdateTerrainNodeDescriptorRequest {
	public:
		uint32_t dstTerrainNodeIndex;					// Ŀ����νڵ�����

		TerrainNodeDescriptor terrainNodeDescriptor;	// Ŀ����νڵ��µ�����
	};

	/*
	* ��̨��(���𴴽��������νڵ��ȫ���������������νڵ����ݡ�����GPUפ���ĵ�������������)
	*/
	class TerrainBackend {
	public:
		TerrainBackend(
			TerrainRenderer* renderer,
			TerrainSetting& terrainSetting,
			std::vector<TerrainLodDescriptor>&  terrainLodDescriptors,
			std::vector<TerrainNodeDescriptor>& terrainNodeDescriptors,
			std::vector<TerrainNodeRuntimeState>& terrainNodeRuntimeStates);

		~TerrainBackend();

		// Ԥ����
		void Preload();

	private:
		// ��̨�߳�
		void BackendThread();

		void CreateGraphicsObject();

	private:
		TerrainRenderer* mRenderer{ nullptr };

		inline static uint32_t smMaxBackFrameCount = 3u;

		std::unique_ptr<GHL::DirectStorageQueue> mBackDStorageQueue;
		std::unique_ptr<GHL::Fence>			     mBackDStorageFence;

		std::unique_ptr<GHL::CommandQueue>       mBackCopyQueue;
		std::unique_ptr<GHL::Fence>              mBackCopyFence;

		std::unique_ptr<GHL::CommandQueue>       mBackGrahpicsQueue;
		std::unique_ptr<GHL::Fence>		         mBackGraphicsFence;

		std::unique_ptr<GHL::CommandQueue>       mBackComputeQueue;
		std::unique_ptr<GHL::Fence>              mBackComputeFence;

		std::unique_ptr<Renderer::RingFrameTracker>         mBackFrameTracker;
		std::unique_ptr<Renderer::ResourceStateTracker>     mBackResourceStateTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mBackLinearBufferAllocator;		// ������ͨ��PassData
		std::unique_ptr<Renderer::PoolCommandListAllocator> mBackPoolCommandListAllocator;

		std::unique_ptr<Renderer::LinearBufferAllocator>    mBackTempLinearBufferAllocator;		// ����Tile�ڹ����ڴ��е�TempBuffer

		// �߳�ͬ������
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// ��������(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
		std::vector<TerrainLodDescriptor>&    mTerrainLodDescriptors;		// ����ȫLOD����������
		std::vector<TerrainNodeDescriptor>&   mTerrainNodeDescriptors;		// ����ȫ�ڵ�����������
		std::vector<TerrainNodeRuntimeState>& mTerrainNodeRuntimeStates;	// ����ȫ�ڵ�����ʱ״̬

		struct UpdateTerrainNodeDescriptorPassData {
		public:
			uint32_t terrainNodeDescriptorBufferIndex;
			uint32_t updateTerrainNodeDescriptorRequestBufferIndex;
		};
		UpdateTerrainNodeDescriptorPassData mUpdateTerrainNodeDescriptorPassData;
		BufferWrap mUpdateTerrainNodeDescriptorRequestBuffer;	// ���ڸ��µ��νڵ������Ļ���

		// ��ɫ����������
		inline static std::string smUpdateTerrainNodeDescriptorSN = "UpdateTerrainNodeDescriptor";
		inline static uint32_t smThreadSizeInGroup = 8u;
	};

}