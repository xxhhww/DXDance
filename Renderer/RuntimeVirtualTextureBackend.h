#pragma once
#include "Renderer/TerrainRenderer.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceStateTracker.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"
#include "Renderer/RuntimeVirtualTextureAtlasTileCache.h"
#include "Renderer/RuntimeVirtualTexturePageTable.h"

#include "Tools/ConcurrentQueue.h"

namespace Renderer {

	/*
	* ʵʱ���������������
	*/
	struct RuntimeVirtualTextureNodeRequestTask {
	public:
		RuntimeVirtualTextureAtlasTileCache::Node* atlasNode{ nullptr };	// һ��ͼ��Ԫ��

		// ǰһ��
		Math::Int2 prevPagePos{ -1, -1 };
		int32_t    prevPageLevel{ -1 };

		// ��һ��
		Math::Int2 nextPagePos{ -1, -1 };
		int32_t    nextPageLevel{ -1 };
	};

	class RuntimeVirtualTextureBackend {
	public:
		struct RecordedGpuCommand {
		public:
			GHL::CommandQueue* graphicsQueue{ nullptr };
			GHL::Fence*        graphicsFence{ nullptr };
			GHL::CommandList*  graphicsCommandList{ nullptr };
			uint64_t           graphicsFenceExpectedValue{ 0u };

			GHL::CommandQueue* computeQueue{ nullptr };
			GHL::Fence*        computeFence{ nullptr };
			GHL::CommandList*  computeCommandList{ nullptr };
			uint64_t           computeFenceExpectedValue{ 0u };

			uint32_t           frameIndex{ 0u };
		};

	public:
		RuntimeVirtualTextureBackend(TerrainRenderer* renderer, TerrainSetting& terrainSetting);

		~RuntimeVirtualTextureBackend();

	private:
		// ��̨�߳�
		void BackendThread();

		// ����Feedback
		void ProcessTerrainFeedback(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks, uint32_t completedFenceValue);

		// ¼��GPU����
		void RecordGpuCommand(std::vector<RuntimeVirtualTextureNodeRequestTask>& requestTasks, RecordedGpuCommand& recordedGpuCommand);

		// ����ͼ�ζ���
		void CreateGraphicsObject();

		// ����֡��ɻص�����
		void SetupFrameCompletedCallBack();

		// ֡��ɻص�
		void OnFrameCompleted(uint8_t frameIndex);

	private:
		TerrainRenderer* mRenderer{ nullptr };

		inline static uint32_t smMaxRvtFrameCount = 3u;

		// �߳�ͬ������
		std::thread mThread;
		HANDLE mHasPreloaded;
		bool mThreadRunning{ true };

		// ��������(From TerrainRenderer)
		TerrainSetting& mTerrainSetting;
		std::vector<RuntimeVirtualTexturePageTable>& mRvtPageTables;

		std::unique_ptr<GHL::CommandQueue>                  mRvtGraphicsQueue;
		std::unique_ptr<GHL::Fence>                         mRvtGraphicsFence;
		std::unique_ptr<Renderer::RingFrameTracker>	        mRvtFrameTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mRvtLinearBufferAllocator;
		std::unique_ptr<Renderer::PoolCommandListAllocator> mRvtCommandListAllocator;

		std::unique_ptr<Renderer::ResourceStateTracker>     mRvtResourceStateTracker;

		struct UpdateVirtualTextureAtlasPassData {

		};

		struct UpdatePageTablePassData {

		};

		UpdateVirtualTextureAtlasPassData mUpdateVirtualTextureAtlasPassData;
		UpdatePageTablePassData mUpdatePageTablePassData;

		Tool::ConcurrentQueue<RecordedGpuCommand> mRecordedGpuCommands;				// �ö�����BackThread��MainThread��ͬ����

		std::vector<std::vector<RuntimeVirtualTextureNodeRequestTask>> mReservedTerrainNodeRequestTasks;	// Ԥ���ĵ��νڵ����������Ա�֡��ɺ�Ļص�����

	};

}