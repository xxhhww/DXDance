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
	public:
		struct UpdateTiledTexturePassData {
		public:

		};

		struct UpdateLookUpPassData {
		public:
			uint32_t rvtDrawLookUpMapRequestBufferIndex;
			float pad1;
			float pad2;
			float pad3;
		};

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
		void UpdateTiledTexturePass(CommandBuffer& commandBuffer);

		/*
		* ����LookUp��ͼ
		*/
		void UpdateLookUpMapPass(CommandBuffer& commandBuffer);

	private:
		void ActivateCell(int x, int y, int mipLevel);

		void LoadPage(int x, int y, int mipLevel);

	private:
		RenderEngine*  mRenderEngine{ nullptr };
		TerrainSystem* mTerrainSystem{ nullptr };
		ShaderManger*  mMainShaderManger{ nullptr };
		ResourceStateTracker* mMainResourceStateTracker{ nullptr };

		RvtTiledTexture* mRvtTiledTexture{ nullptr };

		uint32_t mMaxRvtFrameCount{ 3u };
		uint32_t mTableSize;
		uint32_t mMaxMipLevel;

		bool mThreadRunning{ true };
		HANDLE mFrameCompletedEvent{ nullptr };
		std::thread mProcessThread;

		RvtPageTable* mPageTable{ nullptr };
		std::unordered_map<Math::Int2, RvtPageTableNodeCell, Math::HashInt2> mActiveCells;	// First: TiledTexture��TilePos, Second: �����Rvt��Cell
		std::vector<DrawTileRequest> mPendingDrawTileRequests;

		std::mutex  mRvtLookUpMapMutex;				// ��Ⱦ���̺߳�Process�̶߳�Ҫʹ�õ�RvtLookUpMap��Դ
		TextureWrap mRvtLookUpMap;					// RvtLookUpMap(���Ŀ��Cell����Ⱦ����δ���ɣ���ʹ�����������Ч�ĸ��ڵ����Ⱦ����)
		BufferWrap  mRvtDrawLookUpMapRequestBuffer;	// ʵ��������

		UpdateTiledTexturePassData mUpdateTiledTexturePass;
		UpdateLookUpPassData       mUpdateLookUpPassData;

		std::unique_ptr<GHL::CommandQueue>               mRvtGrahpicsQueue;
		std::unique_ptr<GHL::Fence>                      mRvtFrameFence;

		std::unique_ptr<Renderer::RingFrameTracker>         mRvtFrameTracker;
		std::unique_ptr<Renderer::ResourceStateTracker>     mRvtResourceStateTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mRvtLinearBufferAllocator;
		std::unique_ptr<Renderer::PoolCommandListAllocator> mRvtPoolCommandListAllocator;

		std::unique_ptr<Renderer::Mesh> quadMesh;
	};

}