#pragma once
#include <Windows.h>
#include <thread>
#include <vector>
#include <memory>

#include "Renderer/ResourceAllocator.h"
#include "Renderer/RvtPageTable.h"

#include "Renderer/RingFrameTracker.h"
#include "Renderer/LinearBufferAllocator.h"

#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"
#include "GHL/CommandAllocator.h"
#include "GHL/CommandList.h"

namespace Renderer {

	class TerrainSystem;
	class RvtTiledTexture;

	class RvtUpdater {
	public:
		RvtUpdater(TerrainSystem* terrainSystem);
		~RvtUpdater();

		/*
		* ֪ͨProcess�߳̽��д���
		*/
		void SetFrameCompletedEvent();

	private:

		/*
		* Process�߳�
		*/
		void ProcessThread();

		/*
		* ����Feedback��Readback
		*/
		void ProcessReadback();

		/*
		* ִ����Ⱦ����
		*/
		void ProcessDrawRequests();

		/*
		* ����LookUp��ͼ
		*/
		void UpdateLookUpMap();

	private:
		void ActivateCell(int x, int y, int mipLevel);

		void LoadPage(int x, int y, int mipLevel);

	private:
		RenderEngine* mRenderEngine{ nullptr };
		TerrainSystem* mTerrainSystem{ nullptr };
		RvtTiledTexture* mRvtTiledTexture{ nullptr };

		uint32_t mMaxRvtFrameCount{ 3u };
		uint32_t mTableSize;
		uint32_t mMaxMipLevel;

		bool mThreadRunning{ true };
		HANDLE mFrameCompletedEvent{ nullptr };
		std::thread mProcessThread;

		RvtPageTable* mPageTable{ nullptr };
		std::unordered_map<Math::Int2, RvtPageTableNodeCell> mActiveNodes;	// First: TiledTexture��TilePos, Second: �����Rvt��Cell
		std::vector<DrawTileRequest> mPendingDrawTileRequests;

		std::mutex  mRvtLookUpMapMutex;	// ��Ⱦ���̺߳�Process�̶߳�Ҫʹ�õ�RvtLookUpMap��Դ
		TextureWrap mRvtLookUpMap;		// RvtLookUpMap(���Ŀ��Cell����Ⱦ����δ���ɣ���ʹ�����������Ч�ĸ��ڵ����Ⱦ����)

		std::unique_ptr<GHL::CommandQueue>               mRvtGrahpicsQueue;
		std::unique_ptr<GHL::Fence>                      mRvtFrameFence;
		std::unique_ptr<GHL::CommandAllocator>			 mRvtCommandListAllocator;
		std::unique_ptr<GHL::CommandList>				 mRvtCommandList;

		std::unique_ptr<Renderer::RingFrameTracker>      mRvtFrameTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator> mRvtLinearBufferAllocator;

	};

}