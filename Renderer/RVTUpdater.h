#pragma once
#include <Windows.h>
#include <thread>
#include <vector>
#include <memory>

#include "Renderer/ResourceAllocator.h"
#include "Renderer/RvtPageTable.h"

#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceStateTracker.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"

#include "Renderer/CommandBuffer.h"

#include "Renderer/Mesh.h"

#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"

namespace Renderer {

	class TerrainSystem;
	class ShaderManger;
	class RvtTiledTexture;
	class RenderEngine;

	class RvtUpdater {
		friend class TerrainSystem;
	public:
		struct UpdateRvtTiledTexturePassData {
		public:
			uint32_t terrainHeightMapIndex;
			uint32_t terrainNormalMapIndex;
			uint32_t terrainSplatMapIndex;
			float heightScale;

			uint32_t rChannelAlbedoMapIndex;
			uint32_t rChannelNormalMapIndex;
			uint32_t rChannelRoughnessMapIndex;
			uint32_t rChannelHeightMapIndex;

			uint32_t gChannelAlbedoMapIndex;
			uint32_t gChannelNormalMapIndex;
			uint32_t gChannelRoughnessMapIndex;
			uint32_t gChannelHeightMapIndex;

			uint32_t bChannelAlbedoMapIndex;
			uint32_t bChannelNormalMapIndex;
			uint32_t bChannelRoughnessMapIndex;
			uint32_t bChannelHeightMapIndex;

			uint32_t aChannelAlbedoMapIndex;
			uint32_t aChannelNormalMapIndex;
			uint32_t aChannelRoughnessMapIndex;
			uint32_t aChannelHeightMapIndex;

			uint32_t drawRequestBufferIndex;
			float pad1;
			float pad2;
			float pad3;
		};

		struct UpdateRvtLookUpMapPassData {
		public:
			uint32_t drawRequestBufferIndex;
			float pad1;
			float pad2;
			float pad3;
		};

	public:
		RvtUpdater(TerrainSystem* terrainSystem);
		~RvtUpdater();

		/*
		* 通知Process线程进行处理
		*/
		void SetFrameCompletedEvent();

		inline const auto& GetTableSize()     const { return mTableSize; }
		inline const auto& GetRvtRadius()     const { return mRvtRadius; }
		inline const auto& GetCellSize()      const { return mCellSize; }
		inline const auto& GetChangeViewDis() const { return mChangeViewDis; }
		inline const auto& GetMaxMipLevel()   const { return mMaxMipLevel; }
		inline const auto& GetCurrRvtRect()   const { return mCurrRvtRect; }

		inline const auto GetFixedPos(const Math::Vector3& pos) const {
			return Math::Int2{
				(int)std::floor(pos.x / mCellSize + 0.5f) * (int)mCellSize,
				(int)std::floor(pos.z / mCellSize + 0.5f) * (int)mCellSize
			};
		}
		inline const auto GetFixedCenter(const Math::Int2& pos) const {
			return Math::Int2{
				(int)std::floor(pos.x / mChangeViewDis + 0.5f) * (int)mChangeViewDis,
				(int)std::floor(pos.y / mChangeViewDis + 0.5f) * (int)mChangeViewDis
			};
		}

	private:

		/*
		* Process线程
		*/
		void ProcessThread();

		/*
		* 处理Feedback的Readback
		*/
		void ProcessReadback(uint64_t completedFenceValue);

		/*
		* 执行渲染操作
		*/
		void UpdateTiledTexturePass();

		/*
		* 更新LookUp贴图
		*/
		void UpdateRvtLookUpMapPass();

	private:
		void ActivateCell(int x, int y, int mipLevel);

		void LoadPage(int x, int y, int mipLevel);

	private:
		RenderEngine*  mRenderEngine{ nullptr };
		TerrainSystem* mTerrainSystem{ nullptr };
		ShaderManger*  mMainShaderManger{ nullptr };
		ResourceStateTracker* mMainResourceStateTracker{ nullptr };
		PoolDescriptorAllocator* mMainDescriptorAllocator{ nullptr };

		RvtTiledTexture* mRvtTiledTexture{ nullptr };

		uint32_t mMaxRvtFrameCount;
		uint32_t mTableSize;
		uint32_t mMaxMipLevel;
		float mRvtRadius;		// 虚拟纹理的半径
		float mCellSize;		// Cell的大小
		float mChangeViewDis;

		Math::Vector4 mCurrRvtRect;	// 每次循环都更新

		bool mThreadRunning{ true };
		HANDLE mFrameCompletedEvent{ nullptr };
		std::thread mProcessThread;

		RvtPageTable* mPageTable{ nullptr };
		std::unordered_map<Math::Int2, RvtPageTableNodeCell, Math::HashInt2> mActiveCells;	// First: TiledTexture的TilePos, Second: 抽象的Rvt的Cell
		
		uint32_t mLimitPerFrame;	// 一帧最大的更新个数
		std::vector<std::vector<DrawTileRequest>> mPendingDrawTileRequests;

		std::mutex  mRvtLookUpMapMutex;				// 渲染主线程和Process线程都要使用到RvtLookUpMap资源
		TextureWrap mRvtLookUpMap;					// RvtLookUpMap(如果目标Cell的渲染数据未生成，则使用其最近且有效的父节点的渲染数据)
		
		BufferWrap  mRvtDrawTiledMapRequestsBuffer; // 实例化数据
		BufferWrap  mRvtDrawLookUpMapRequestBuffer;	// 实例化数据

		UpdateRvtTiledTexturePassData mUpdateRvtTiledMapPassData;
		UpdateRvtLookUpMapPassData    mUpdateRvtLookUpMapPassData;

		std::unique_ptr<GHL::CommandQueue>               mRvtGrahpicsQueue;
		std::unique_ptr<GHL::Fence>                      mRvtFrameFence;

		std::unique_ptr<Renderer::RingFrameTracker>         mRvtFrameTracker;
		std::unique_ptr<Renderer::ResourceStateTracker>     mRvtResourceStateTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mRvtLinearBufferAllocator;
		std::unique_ptr<Renderer::PoolCommandListAllocator> mRvtPoolCommandListAllocator;

		std::unique_ptr<Renderer::Mesh> quadMesh;
	};

}