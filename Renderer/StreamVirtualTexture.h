#pragma once
#include "GHL/DirectStorageFactory.h"
#include "GHL/DirectStorageQueue.h"
#include "Renderer/XeTextureFormat.h"
#include "Renderer/FileHandle.h"
#include "Renderer/RingFrameTracker.h"
#include "Renderer/ResourceAllocator.h"
#include "Renderer/BuddyHeapAllocator.h"

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
			PoolDescriptorAllocator* mainPooldescriptorAllocator,
			BuddyHeapAllocator* mainBuddyheapAllocator,
			const XeTexureFormat& fileFormat,
			std::unique_ptr<FileHandle>&& fileHandle
		);

		~StreamVirtualTexture();


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
		// Graphics Object
		const GHL::Device* mDevice{ nullptr };
		TileUploader* mTileUploader{ nullptr };
		RingFrameTracker* mMainRingFrameTracker{ nullptr };					// 主渲染线程的帧追踪器
		ResourceAllocator* mMainResourceAllocator{ nullptr };				// 创建InternalTexture
		BuddyHeapAllocator* mMainBuddyHeapAllocator{ nullptr };				// 显存堆分配器
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
		std::vector<uint8_t> mMinMipMapCache;
		std::vector<BYTE, AlignedAllocator<BYTE>> mMinMipMap; // 该纹理的MipLevel驻留信息，总是需要更新到GlobalMinMipMap中
		uint64_t mResidencyMapOffset{ 0u }; // 该纹理的驻留信息在全局驻留信息中的偏移量(索引)

		/*
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
				Evicting = 3
			};
		public:
			TileMappingState(uint32_t mipNums, std::vector<D3D12_SUBRESOURCE_TILING>& subresourceTilings);
			~TileMappingState();

			void SetRefCount(uint32_t x, uint32_t y, uint32_t s, uint32_t refCount);
			void IncRefCount(uint32_t x, uint32_t y, uint32_t s);
			void DecRefCount(uint32_t x, uint32_t y, uint32_t s);
			void SetResidencyState(uint32_t x, uint32_t y, uint32_t s, ResidencyState state);
			void SetHeapAllocation(uint32_t x, uint32_t y, uint32_t s, BuddyHeapAllocator::Allocation* buddyHeapAllocation);

			inline const auto& GetRefCount(uint32_t x, uint32_t y, uint32_t s)       const { return mRefCounts[s][y][x]; }
			inline const auto& GetResidencyState(uint32_t x, uint32_t y, uint32_t s) const { return mResidencyStates[s][y][x]; }
			inline auto* GetHeapAllocation(uint32_t x, uint32_t y, uint32_t s)       const { return mBuddyHeapAllocations[s][y][x]; }

		private:
			TileSeq<uint32_t> mRefCounts;								// 每一个Tile的引用计数
			TileSeq<ResidencyState> mResidencyStates;					// 每一个Tile的驻留状态
			TileSeq<BuddyHeapAllocator::Allocation*> mBuddyHeapAllocations;	// 每一个Tile的分配信息
		};
		std::unique_ptr<TileMappingState> mTileMappingState;

		std::vector<D3D12_TILED_RESOURCE_COORDINATE> mPendingTileLoadings;

		class EvictionDelay {
		public:
			EvictionDelay(uint32_t nFrames = 3u);
			~EvictionDelay() = default;

			void Append(D3D12_TILED_RESOURCE_COORDINATE coord) { mEvictionsBuffer[0].push_back(coord); }
			std::vector<D3D12_TILED_RESOURCE_COORDINATE>& GetReadyToEvict() { return mEvictionsBuffer.back(); }

			void MoveToNextFrame();
			void Clear();

			// drop pending evictions for tiles that now have non-zero refcount
			void Rescue(const TileMappingState& in_tileMappingState);

		private:
			std::vector<std::vector<D3D12_TILED_RESOURCE_COORDINATE>> mEvictionsBuffer;
		};
		EvictionDelay mPendingTileEvictions;
		*/


	};

}