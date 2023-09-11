#pragma once
#include <Windows.h>
#include <thread>
#include <vector>
#include <memory>

#include "Renderer/ResourceAllocator.h"
#include "Renderer/RvtPageTable.h"
#include "Renderer/RvtTiledTexture.h"

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
	class RenderEngine;

	class RvtUpdater {
		friend class TerrainSystem;
	public:
		enum class RvtFrameType : uint32_t {
			ProcessFeedbackFrame = 0x01,	// 处理Feedback的帧
			AftermathFrame		 = 0x02		// 善后帧
		};

		struct DrawTileRequest {
		public:
			int x{ 0 };
			int y{ 0 };
			int mipLevel{ 0 };

			Math::Int2 tilePos{ -1, -1 };
		public:
			inline DrawTileRequest(int x, int y, int mipLevel) : x(x), y(y), mipLevel(mipLevel) {}
		};

		struct CellIndex {
		public:
			int x{ 0 };
			int y{ 0 };
			int mipLevel{ 0 };

		public:
			inline CellIndex(int x, int y, int mipLevel) : x(x), y(y), mipLevel(mipLevel) {}
		};

		struct UpdateRvtLookUpMapPassData {
		public:
			uint32_t drawRequestBufferIndex;
			float pad1;
			float pad2;
			float pad3;
		};

		struct UpdateRvtTiledMapPassData {
		public:
			uint32_t drawRequestBufferIndex;
			uint32_t terrainHeightMapIndex;
			uint32_t terrainNormalMapIndex;
			uint32_t terrainSplatMapIndex;

			Math::Vector2 terrainMeterSize;
			float terrainHeightScale;
			float pad;

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
		};

	public:
		RvtUpdater(TerrainSystem* terrainSystem);
		~RvtUpdater();

		/*
		* 通知Process线程进行处理
		*/
		void SetFrameCompletedEvent();

		inline auto* GetRvtTiledMap() { return mRvtTiledTexture.get(); }
		inline const auto* GetRvtTiledMap() const { return mRvtTiledTexture.get(); }

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

		void RetireTilePos(const Math::Int2& tilePos);

		/*
		* RvtFrame帧完成后执行的回调函数
		*/
		void OnRvtFrameCompleted(const RingFrameTracker::FrameAttribute& attribute, uint64_t completedValue);

	private:
		RenderEngine*  mRenderEngine{ nullptr };
		TerrainSystem* mTerrainSystem{ nullptr };

		ShaderManger*  mMainShaderManger{ nullptr };
		ResourceStateTracker* mMainResourceStateTracker{ nullptr };
		PoolDescriptorAllocator* mMainDescriptorAllocator{ nullptr };

		std::unique_ptr<RvtTiledTexture> mRvtTiledTexture;
		std::unique_ptr<RvtPageTable> mPageTable;
		std::unordered_map<Math::Int2, CellIndex, Math::HashInt2> mActiveCells;	// First: TiledTexture的TilePos, Second: 抽象的Rvt的Cell的Index

		uint32_t mMaxRvtFrameCount;
		uint32_t mTableSize;
		uint32_t mMaxMipLevel;
		float mRvtRadius;		// 虚拟纹理的半径
		float mCellSize;		// Cell的大小
		float mChangeViewDis;

		Math::Vector2 mCurrRvtRectCenter;
		Math::Vector4 mCurrRvtRect;

		bool mThreadRunning{ true };
		HANDLE mFrameCompletedEvent{ nullptr };
		std::thread mProcessThread;

		std::mutex  mRvtLookUpMapMutex;	// 渲染主线程和Process线程都要使用到RvtLookUpMap资源
		TextureWrap mRvtLookUpMap;		// RvtLookUpMap(如果目标Cell的渲染数据未生成，则使用其最近且有效的父节点的渲染数据)
		
		uint32_t mLimitPerFrame;		// 一帧最大的更新个数
		std::vector<std::vector<DrawTileRequest>> mLoadingDrawTileRequests;	// GPU正在处理的DrawTileRequests
		std::vector<std::vector<DrawTileRequest>> mPendingDrawTileRequests;

		BufferWrap  mRvtDrawLookUpMapRequestBuffer;	// 实例化数据
		BufferWrap  mRvtDrawTiledMapRequestsBuffer; // 实例化数据

		UpdateRvtLookUpMapPassData mUpdateRvtLookUpMapPassData;
		UpdateRvtTiledMapPassData  mUpdateRvtTiledMapPassData;

		std::unique_ptr<GHL::CommandQueue>               mRvtGrahpicsQueue;
		std::unique_ptr<GHL::Fence>                      mRvtFrameFence;

		std::unique_ptr<Renderer::RingFrameTracker>         mRvtFrameTracker;
		std::unique_ptr<Renderer::ResourceStateTracker>     mRvtResourceStateTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mRvtLinearBufferAllocator;
		std::unique_ptr<Renderer::PoolCommandListAllocator> mRvtPoolCommandListAllocator;

		std::unique_ptr<Renderer::Mesh> quadMesh;
	};

}