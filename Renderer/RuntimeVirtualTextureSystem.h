#pragma once
#include "Renderer/PageTable.h"
#include "Renderer/RvtPhysicalTexture.h"
#include <thread>
#include <memory>

#include "GHL/CommandQueue.h"
#include "GHL/Fence.h"

#include "Renderer/ResourceAllocator.h"
#include "Renderer/PageTable.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceStateTracker.h"
#include "Renderer/LinearBufferAllocator.h"
#include "Renderer/PoolCommandListAllocator.h"
#include "Renderer/CommandBuffer.h"
#include "Renderer/Mesh.h"

namespace Renderer {

	class RvtPhysicalTexture;
	class TerrainSystem;
	class ShaderManger;
	class RenderEngine;

	class RuntimeVirtualTextureSystem {
		friend class RvtPhysicalTexture;
	public:
		struct Task {
		public:
			TileCache::Node* node{ nullptr };

			// ǰһ��
			Math::Int2 prevPage0Pos{ -1, -1 };	// ��PageLevelTable0�ϵ�λ��
			int32_t    prevMipLevel{ -1 };

			// ��һ��
			Math::Int2 nextPage0Pos{ -1, -1 };	// ��PageLevelTable0�ϵ�λ��
			int32_t    nextMipLevel{ -1 };

		public:
			inline Task(const Math::Int2& nextPage0Pos, int32_t nextMipLevel) 
			: nextPage0Pos(nextPage0Pos)
			, nextMipLevel(nextMipLevel) {}

			~Task() = default;
		};

	public:
		struct UpdatePhysicalTexturePassData {
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

		struct UpdatePageTableTexturePassData {
		public:
			uint32_t drawRequestBufferIndex;
			float pad1;
			float pad2;
			float pad3;
		};

	public:
		RuntimeVirtualTextureSystem(TerrainSystem* terrainSystem);
		~RuntimeVirtualTextureSystem();

		inline const auto& GetTileSize()         const { return mTileSize; }
		inline const auto& GetPaddingSize()      const { return mPaddingSize; }
		inline const auto& GetTileCountPerAxis() const { return mTileCountPerAxis; }
		inline const auto& GetTileCount()        const { return mTileCount; }
		inline const auto& GetVTSize()           const { return mVirtualTextureSize; }
		inline const auto& GetMaxMipLevel()      const { return mMaxMipLevel; }
		inline const auto& GetCurrRvtRect()      const { return mCurrRvtRect; }

		inline const auto GetPhysicalTextureSize() const { return mRvtPhysicalTexture->GetPhysicalTextureSize(); }
		inline const auto GetTileSizeWithPadding() const { return mTileSize + 2u * mPaddingSize; }

		inline void LockGPUResource()	{ mMutex.lock(); }
		inline void UnlockGPUResource() { mMutex.unlock(); }

	private:
		// ��ʼ��ͼ�ζ���
		void InitializeGraphicsObject(TerrainSystem* terrainSystem);

		// �����߳�
		void UpdateThread();

		// ����Feedback
		void ProcessFeedback(uint64_t completedFenceValue);

		// ������������
		void UpdatePhysicalTexturePass();

		// ����ҳ������
		void UpdatePageTableTexturePass();

		// �����������
		void ClearTaskQueue();

		// ����Tile
		void DistributeTile(int page0PosX, int page0PosY, int mipLevel);

		// һ������֡��ɺ�Ļص�
		void OnRvtFrameCompleted(uint8_t rvtFrameIndex);

	private:
		TerrainSystem* mTerrainSystem{ nullptr };
		RenderEngine*  mRenderEngine{ nullptr };

		// From MainRenderEngine
		ShaderManger*  mMainShaderManger{ nullptr };
		ResourceStateTracker*    mMainResourceStateTracker{ nullptr };
		PoolDescriptorAllocator* mMainDescriptorAllocator{ nullptr };

		// Graphics API Object
		uint32_t mMaxRvtFrameCount;
		std::unique_ptr<GHL::CommandQueue> mRvtGrahpicsQueue;
		std::unique_ptr<GHL::Fence>        mRvtFrameFence;

		std::unique_ptr<Renderer::RingFrameTracker>         mRvtFrameTracker;
		std::unique_ptr<Renderer::ResourceStateTracker>     mRvtResourceStateTracker;
		std::unique_ptr<Renderer::LinearBufferAllocator>    mRvtLinearBufferAllocator;
		std::unique_ptr<Renderer::PoolCommandListAllocator> mRvtPoolCommandListAllocator;

		// Quad Mesh
		std::unique_ptr<Renderer::Mesh> quadMesh;

		// Thread Parameters
		bool mThreadRunning{ true };
		std::thread mProcessThread;
		std::mutex mMutex;

		// Misc Parameters(����������������)
		uint32_t mTileSize;				// ����������Tile�ĳߴ��С
		uint32_t mPaddingSize;			// Tile����������ı߿����
		uint32_t mTileCountPerAxis;		// ����������ÿ�����Tile��ƽ�̸���
		uint32_t mTileCount;			// ����������Tile���ܸ���
		uint32_t mVirtualTextureSize;	// ��������ĳߴ��С(mTileSize * mTileCount)

		uint32_t mMaxMipLevel;

		Math::Vector4 mCurrRvtRect;

		// Rvtҳ��
		std::unique_ptr<PageTable> mPageTable;

		// ��������
		std::unique_ptr<RvtPhysicalTexture> mRvtPhysicalTexture;
		
		// �������
		std::vector<Task> mTasks;
		uint32_t mLimitTaskPerFrame;	// ÿһ֡���ƴ�����������
		std::vector<std::vector<Task>> mLoadingTasks;	// ÿһ֡�ύ�ļ�������

		TextureWrap mPageTableTexture;

		BufferWrap  mDrawPhysicalTextureRequestBuffer;		// ʵ��������
		BufferWrap  mDrawPageTableTextureRequestBuffer;		// ʵ��������

		UpdatePhysicalTexturePassData mUpdatePhysicalTexturePassData;
		UpdatePageTableTexturePassData mUpdatePageTableTexturePassData;
	};
}