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
		* 通知Process线程进行处理
		*/
		void SetFrameCompletedEvent();

	private:

		/*
		* Process线程
		*/
		void ProcessThread();

		/*
		* 处理Feedback的Readback
		*/
		void ProcessReadback();

		/*
		* 执行渲染操作
		*/
		void ProcessDrawRequests();

		/*
		* 更新LookUp贴图
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
		std::unordered_map<Math::Int2, RvtPageTableNodeCell> mActiveNodes;	// First: TiledTexture的TilePos, Second: 抽象的Rvt的Cell
		std::vector<DrawTileRequest> mPendingDrawTileRequests;

		std::mutex  mRvtLookUpMapMutex;	// 渲染主线程和Process线程都要使用到RvtLookUpMap资源
		TextureWrap mRvtLookUpMap;		// RvtLookUpMap(如果目标Cell的渲染数据未生成，则使用其最近且有效的父节点的渲染数据)

		std::unique_ptr<GHL::CommandQueue>               mRvtGrahpicsQueue;
		std::unique_ptr<GHL::Fence>                      mRvtFrameFence;
		std::unique_ptr<GHL::CommandAllocator>			 mRvtCommandListAllocator;
		std::unique_ptr<GHL::CommandList>				 mRvtCommandList;

		std::unique_ptr<Renderer::RingFrameTracker>      mRvtFrameTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator> mRvtLinearBufferAllocator;

	};

}