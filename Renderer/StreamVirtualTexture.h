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

		// ����Feedback
		void ProcessFeedback(uint64_t completedFenceValue);

		// ����PendingTileLoadings
		uint32_t ProcessTileLoadings();

		// ����PendingTileEvictions
		uint32_t ProcessTileEvictions();

		// PendingTileLoadings��ɺ�Ļص�����
		void OnCopyCompleted(std::vector<D3D12_TILED_RESOURCE_COORDINATE>& coords);

		// ����פ��MinMip��Ϣ
		void UpdateMinMipMap();

		// ¼��Feedback����������
		void RecordClearFeedback(CommandBuffer& commandBuffer);

		// ¼�ƽ���Feedback������
		void RecordResolve(CommandBuffer& commandBuffer);

		// ¼�ƻض�ResolvedResource������
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
		inline static uint32_t smMaxTileCountPerLoop = 64u;	// ÿһ��ѭ��Ҫ��������Tile���� 
		// Graphics Object
		const GHL::Device* mDevice{ nullptr };
		TileUploader* mTileUploader{ nullptr };
		RingFrameTracker* mMainRingFrameTracker{ nullptr };					// ����Ⱦ�̵߳�֡׷����
		ResourceAllocator* mMainResourceAllocator{ nullptr };				// ����InternalTexture
		BuddyHeapAllocator* mMainBuddyHeapAllocator{ nullptr };				// �Դ�ѷ�����
		ResourceStateTracker* mMainResourceStateTracker{ nullptr };			// ��Դ״̬׷����
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
		std::atomic<bool> mResidencyChanged{ false };	// ��MonitorThread��UpdateResidencyThread�������޸�
		std::vector<uint8_t> mMinMipMapCache;
		std::vector<BYTE, AlignedAllocator<BYTE>> mMinMipMap; // �������MipLevelפ����Ϣ��������Ҫ���µ�GlobalMinMipMap��
		uint64_t mResidencyMapOffset{ 0u }; // �������פ����Ϣ��ȫ��פ����Ϣ�е�ƫ����(����)

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
			TileSeq<uint32_t> mRefCounts;								// ÿһ��Tile�����ü���
			TileSeq<ResidencyState> mResidencyStates;					// ÿһ��Tile��פ��״̬
			TileSeq<BuddyHeapAllocator::Allocation*> mHeapAllocations;	// ÿһ��Tile�ķ�����Ϣ
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