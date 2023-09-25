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
		RingFrameTracker* mMainRingFrameTracker{ nullptr };					// ����Ⱦ�̵߳�֡׷����
		ResourceAllocator* mMainResourceAllocator{ nullptr };				// ����InternalTexture
		BuddyHeapAllocator* mMainBuddyHeapAllocator{ nullptr };				// �Դ�ѷ�����
		PoolDescriptorAllocator* mMainPoolDescriptorAllocator{ nullptr };	// ������������
		TextureWrap mInternalTexture;

		// Sampler Feedback Resource(��TextureWrap�����ݣ�ʹ��ԭʼAPI)
		D3D12_RESOURCE_DESC1 mFeedbackResourceDesc{};
		Microsoft::WRL::ComPtr<ID3D12Resource2> mFeedbackTexture;
		DescriptorHandleWrap mFeedbackUADescriptor;
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mClearCpuUavHeap;

		TextureWrap mResolvedTexture;

		// �ض���Դ����(��SVT�߳�����Ⱦ���̹߳�ͬ����)
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
		std::vector<BYTE, AlignedAllocator<BYTE>> mMinMipMap; // �������MipLevelפ����Ϣ��������Ҫ���µ�GlobalMinMipMap��
		uint64_t mResidencyMapOffset{ 0u }; // �������פ����Ϣ��ȫ��פ����Ϣ�е�ƫ����(����)

		/*
		// Tileҳ��
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
			TileSeq<uint32_t> mRefCounts;								// ÿһ��Tile�����ü���
			TileSeq<ResidencyState> mResidencyStates;					// ÿһ��Tile��פ��״̬
			TileSeq<BuddyHeapAllocator::Allocation*> mBuddyHeapAllocations;	// ÿһ��Tile�ķ�����Ϣ
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