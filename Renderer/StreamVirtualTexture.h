#pragma once
#include "GHL/DirectStorageFactory.h"
#include "GHL/DirectStorageQueue.h"

#include "Renderer/XeTextureFormat.h"
#include "Renderer/FileHandle.h"

#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/ResourceStateTracker.h"

#include "Renderer/CommandBuffer.h"

#include "Tools/TemplateAllocator.h"

#include <memory>

namespace Renderer {

	class TileUploader;
	class StreamVirtualTexture {
	public:
		StreamVirtualTexture(
			const GHL::Device* device,
			TileUploader* tileUploader,
			RingFrameTracker* mainRingFrameTracker,
			ResourceAllocator* mainResourceAllocator,
			BuddyHeapAllocator* mainBuddyheapAllocator,
			ResourceStateTracker* mainResourceStateTracker,
			PoolDescriptorAllocator* mainPooldescriptorAllocator,
			const XeTexureFormat& fileFormat,
			std::unique_ptr<FileHandle>&& fileHandle
		);

		~StreamVirtualTexture();

		// 处理Feedback
		void ProcessFeedback(uint64_t completedFenceValue);

		// 处理PendingTileLoadings
		uint32_t ProcessTileLoadings();

		// 处理PendingTileEvictions
		uint32_t ProcessTileEvictions();

		// PendingTileLoadings完成后的回调函数
		void OnCopyCompleted(std::vector<D3D12_TILED_RESOURCE_COORDINATE>& coords);

		// 更新驻留MinMip信息
		void UpdateMinMipMap();

		// 录制Feedback的清理命令
		void RecordClearFeedback(CommandBuffer& commandBuffer);

		// 录制解算Feedback的命令
		void RecordResolve(CommandBuffer& commandBuffer);

		// 录制回读ResolvedResource的命令
		void RecordReadback(CommandBuffer& commandBuffer);

		inline bool IsStale() const { return mPendingTileLoadings.size() || mPendingTileEvictions.GetReadyToEvict().size(); }

		inline auto*       D3DResource()       { return mInternalTexture.Get()->D3DResource(); }
		inline const auto* D3DResource() const { return mInternalTexture.Get()->D3DResource(); }

		inline const auto& GetPackedMipInfo()      const { return mPackedMipInfo; }
		inline const auto& GetTileShape()          const { return mTileShape; }
		inline const auto& GetNumTilesTotal()      const { return mNumTilesTotal; }
		inline const auto& GetTilings()            const { return mTiling; }
		inline const auto& GetTileTexelWidth()     const { return mTileShape.WidthInTexels; }
		inline const auto& GetTileTexelHeight()    const { return mTileShape.HeightInTexels; }
		inline const auto& GetTileTexelDepth()     const { return mTileShape.DepthInTexels; }
		inline const auto& GetNumTilesWidth()      const { return mTiling.at(0).WidthInTiles; }
		inline const auto& GetNumTilesHeight()     const { return mTiling.at(0).HeightInTiles; }
		inline const auto& GetNumTilesDepth()      const { return mTiling.at(0).DepthInTiles; }
		inline const auto& GetResidencyMapOffset() const { return mResidencyMapOffset; }

		inline const auto& GetFileFormat()         const { return mFileFormat; }
		inline const auto* GetFileHandle()         const { return mFileHandle.get(); }

	private:
		void SetMinMip(uint8_t currentMip, uint32_t x, uint32_t y, uint8_t desiredMip);
		void AddTileRef(uint32_t x, uint32_t y, uint32_t s);
		void DecTileRef(uint32_t x, uint32_t y, uint32_t s);

		void RescanPendingTileLoads();

		void SetResidencyChanged();

	private:
		inline static uint32_t smTileSize = 65536u;
		inline static uint32_t smMaxTileCountPerLoop = 64u;	// 每一次循环要处理的最大Tile个数 
		// Graphics Object
		const GHL::Device* mDevice{ nullptr };
		TileUploader* mTileUploader{ nullptr };
		RingFrameTracker* mMainRingFrameTracker{ nullptr };					// 主渲染线程的帧追踪器
		ResourceAllocator* mMainResourceAllocator{ nullptr };				// 创建InternalTexture
		BuddyHeapAllocator* mMainBuddyHeapAllocator{ nullptr };				// 显存堆分配器
		ResourceStateTracker* mMainResourceStateTracker{ nullptr };			// 资源状态追踪器
		PoolDescriptorAllocator* mMainPoolDescriptorAllocator{ nullptr };	// 描述符分配器
		TextureWrap mInternalTexture;

		// Sampler Feedback Resource(与TextureWrap不兼容，使用原始API)
		D3D12_RESOURCE_DESC1 mFeedbackResourceDesc{};
		Microsoft::WRL::ComPtr<ID3D12Resource2> mFeedbackTexture;
		DescriptorHandleWrap mFeedbackUADescriptor;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mClearCpuUavHeap;

		TextureWrap mResolvedTexture;

		// 回读资源队列(由SVT线程与渲染主线程共同访问)
		struct QueuedReadbackFeedback {
			uint64_t renderFrameFenceValue{ 0u };
			std::atomic<bool> isFresh{ false };
		};
		std::vector<BufferWrap> mReadbackBuffers;
		std::vector<QueuedReadbackFeedback> mQueuedReadbacks;

		// XetFileHandle
		XeTexureFormat mFileFormat;
		std::unique_ptr<FileHandle> mFileHandle;

		// D3D12TilingInformation
		D3D12_PACKED_MIP_INFO mPackedMipInfo; // last n mips may be packed into a single tile
		D3D12_TILE_SHAPE mTileShape;          // e.g. a 64K tile may contain 128x128 texels @ 4B/pixel
		UINT mNumTilesTotal;
		std::vector<D3D12_SUBRESOURCE_TILING> mTiling;
		uint8_t mNumStandardMips{ 0u };	// Start From 1u

		// PackedMipsInformation
		uint32_t mPackedMipsFileOffset{ 0u };
		uint32_t mPackedMipsNumBytes{ 0u };
		uint32_t mPackedMipsUncompressedSize{ 0u };
		BuddyHeapAllocator::Allocation* mPackedMipsHeapAllocation{ nullptr };

		// ResidencyMipMap
		std::atomic<bool> mResidencyChanged{ false };	// 被MonitorThread和UpdateResidencyThread访问与修改
		std::vector<uint8_t> mMinMipMapCache;
		std::vector<BYTE, AlignedAllocator<BYTE>> mMinMipMap; // 该纹理的MipLevel驻留信息，总是需要更新到GlobalMinMipMap中
		uint64_t mResidencyMapOffset{ 0u }; // 该纹理的驻留信息在全局驻留信息中的偏移量(索引)

		// Tile页表
		class TileMappingState {
			friend class StreamTexture;
		public:
			template<typename T> using TileRow = std::vector<T>;
			template<typename T> using TileMip = std::vector<TileRow<T>>;
			template<typename T> using TileSeq = std::vector<TileMip<T>>;

			enum class ResidencyState : uint8_t {
				NotResident = 0,
				Resident = 1,
				Loading = 2,
			};
		public:
			TileMappingState(uint32_t mipNums, std::vector<D3D12_SUBRESOURCE_TILING>& subresourceTilings);
			~TileMappingState();

			inline void SetRefCount(uint32_t x, uint32_t y, uint32_t s, uint32_t refCount) { mRefCounts[s][y][x] = refCount; }
			
			inline void IncRefCount(uint32_t x, uint32_t y, uint32_t s) { mRefCounts[s][y][x] ++; }
			inline void DecRefCount(uint32_t x, uint32_t y, uint32_t s) { mRefCounts[s][y][x] --; }

			inline void SetResidencyState(uint32_t x, uint32_t y, uint32_t s, ResidencyState state) { mResidencyStates[s][y][x] = state; }
			inline void SetHeapAllocation(uint32_t x, uint32_t y, uint32_t s, BuddyHeapAllocator::Allocation* heapAllocation) { mHeapAllocations[s][y][x] = heapAllocation; }

			inline const auto& GetRefCount(uint32_t x, uint32_t y, uint32_t s)       const { return mRefCounts[s][y][x]; }
			inline const auto& GetResidencyState(uint32_t x, uint32_t y, uint32_t s) const { return mResidencyStates[s][y][x]; }
			inline auto* GetHeapAllocation(uint32_t x, uint32_t y, uint32_t s)       const { return mHeapAllocations[s][y][x]; }

		private:
			TileSeq<uint32_t> mRefCounts;								// 每一个Tile的引用计数
			TileSeq<ResidencyState> mResidencyStates;					// 每一个Tile的驻留状态
			TileSeq<BuddyHeapAllocator::Allocation*> mHeapAllocations;	// 每一个Tile的分配信息
		};
		std::unique_ptr<TileMappingState> mTileMappingState;

		std::vector<D3D12_TILED_RESOURCE_COORDINATE> mPendingTileLoadings;

		class EvictionDelay {
		public:
			EvictionDelay(uint32_t nFrames = 3u);
			~EvictionDelay();

			inline void Append(D3D12_TILED_RESOURCE_COORDINATE coord) { mEvictionsBuffer[0].push_back(coord); }

			inline std::vector<D3D12_TILED_RESOURCE_COORDINATE>&       GetReadyToEvict()       { return mEvictionsBuffer.back(); }
			inline const std::vector<D3D12_TILED_RESOURCE_COORDINATE>& GetReadyToEvict() const { return mEvictionsBuffer.back(); }

			void MoveToNextFrame();
			void Clear();

			// drop pending evictions for tiles that now have non-zero refcount
			void Rescue(const TileMappingState& in_tileMappingState);

		private:
			std::vector<std::vector<D3D12_TILED_RESOURCE_COORDINATE>> mEvictionsBuffer;
		};
		EvictionDelay mPendingTileEvictions;
	};

}