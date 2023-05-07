#pragma once
#include "DirectStorage/dstorage.h"

#include "Texture.h"
#include "XeTextureFormat.h"

#include "Tools/TemplateAllocator.h"

#include <atomic>

namespace GHL {
	class CommandQueue;
	class Fence;
}

namespace Renderer {

	class FileHandle;
	class DataUploader;

	class StreamTexture {
	public:
		StreamTexture(
			const GHL::Device* device,
			DataUploader* dataUploader,
			const XeTexureFormat& xeTextureFormat,
			std::unique_ptr<FileHandle> fileHandle,
			PoolDescriptorAllocator* descriptorAllocator,
			BuddyHeapAllocator* heapAllocator,
			RingFrameTracker* ringFrameTracker);

		~StreamTexture();

		/*
		* ¼��Feedback����������
		*/
		void RecordClearFeedback(ID3D12GraphicsCommandList4* commandList);

		/*
		* ¼�ƽ���Feedback������
		*/
		void RecordResolve(ID3D12GraphicsCommandList4* commandList);

		/*
		* ¼�ƻض�ResolvedResource������
		*/
		void RecordReadback(ID3D12GraphicsCommandList4* commandList);

		/*
		* ӳ�䲢���ļ��м���PackedMipMap���ݣ�����Ⱦ���߳�ֱ�ӵ��ã����������̵߳���
		*/
		void MapAndLoadPackedMipMap(GHL::CommandQueue* mappingQueue, GHL::Fence* packedMipMappingFence, IDStorageQueue* copyDsQueue, GHL::Fence* copyFence);

		/*
		* �ú�����������Ⱦ���߳������У�������ProcessFeedback�߳�������
		* ��ÿһ����Ⱦ֡��ɺ���Ⱦ���̻߳�֪ͨProcessFeedback�̶߳�Feedback���д����Ӷ����øú���
		* ��ˣ���Ҫע���߳�֮���ͬ������
		*/
		void ProcessReadbackFeedback();

		/*
		* ����Tile���ص����񣬷���ֵ��ʾ�Ƿ���������Ҫ�ӳ��ύ
		*/
		bool ProcessTileLoadings();

		/*
		* ����Tileж�ص����񣬷���ֵ��ʾ�Ƿ���������Ҫ�ӳ��ύ
		*/
		bool ProcessTileEvictions();

		/*
		* ��Ⱦ֡��ɺ�Ļص���������ֱ��ע�ᵽFrameTracker����Ⱦ֡��ɻص��У�������StreamTextureManger�е�ͬ���ص���������
		*/
		void FrameCompletedCallback(uint8_t frameIndex);

		/*
		* Tile�������ϴ���ӳ����ɺ�Ļص�����
		* @Param coords: ����Ŀ��Tiles
		*/
		void TileLoadingsCompletedCallback(const std::vector<D3D12_TILED_RESOURCE_COORDINATE>& coords);

		/*
		* ���ø�StreamTexture��Residency��ȫ��ResidencyMap�е�ƫ����
		*/
		void SetResidencyMapOffset(uint64_t mapOffset);

		/*
		* ��StreamTexture�Ƿ�������
		*/
		bool IsStale();

		inline auto* GetInternalResource() const { return mInternalTexture; }

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

	private:
		const GHL::Device* mDevice { nullptr };
		DataUploader* mDataUploader{ nullptr };

		// =============================== XetFileHandle ===============================
		XeTexureFormat mFileFormat;
		std::unique_ptr<FileHandle> mFileHandle;

		Texture* mInternalTexture { nullptr };
		RingFrameTracker* mFrameTracker { nullptr };
		BuddyHeapAllocator* mHeapAllocator { nullptr };

		// =============================== D3D12TilingInformation ===============================
		D3D12_PACKED_MIP_INFO mPackedMipInfo; // last n mips may be packed into a single tile
		D3D12_TILE_SHAPE mTileShape;          // e.g. a 64K tile may contain 128x128 texels @ 4B/pixel
		UINT mNumTilesTotal;
		std::vector<D3D12_SUBRESOURCE_TILING> mTiling;
		uint8_t mNumStandardMips{ 0u }; // Start From 1u

		// =============================== PackedMipsInformation ===============================
		uint32_t mPackedMipsFileOffset{ 0u };
		uint32_t mPackedMipsNumBytes{ 0u };
		uint32_t mPackedMipsUncompressedSize{ 0u };
		BuddyHeapAllocator::Allocation* mPackedMipsHeapAllocation{ nullptr };

		// =============================== Feedback & Resolved Information ===============================
		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mClearUavHeap;

		D3D12_RESOURCE_DESC1 mFeedbackResourceDesc{};
		Microsoft::WRL::ComPtr<ID3D12Resource2> mFeedbackResource;
		DescriptorHandleWrap mFeedbackUADescriptor;

		Microsoft::WRL::ComPtr<ID3D12Resource> mResolvedResource;
		
		// =============================== Readback Information ===============================
		struct QueuedReadbackFeedback {
			uint64_t renderFrameFenceValue{ 0u };	// �ñ�������Ⱦ���߳�д�룬ProcessFeedback�߳�ֻ��
			std::atomic<bool> isFresh{ false };		// �ñ�������Ⱦ���߳���ProcessFeedback�߳̽��з������޸�
		};
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mReadbackResource;
		/*
		* ��ÿһ����Ⱦ֡��ɺ���Ⱦ���̶߳������QueuedReadbackFeedback����ṹ������е�isFresh��������֪ͨProcessFeedback�߳̽��д���
		* ����ProcessFeedback�߳��У�Ҳ���isFresh���������޸�
		* ע�⣺��Ⱦ֡��ɺ�Ļص�ע�������StreamTextureStorage�У������Ͳ���Ҫ��ÿһ��StreamTexture������лص�ע����
		*/
		std::vector<QueuedReadbackFeedback> mQueuedReadbackFeedback;

		// =============================== ResidencyMipMap ===============================
		std::vector<uint8_t> mMinMipMapCache;
		std::vector<BYTE, AlignedAllocator<BYTE>> mMinMipMap; // �������MipLevelפ����Ϣ��������Ҫ���µ�GlobalMinMipMap��
		uint64_t mResidencyMapOffset{ 0u }; // �������פ����Ϣ��ȫ��פ����Ϣ�е�ƫ����(����)

	private:
		size_t mTileSize{ 65536u };
		// =============================== TileMappingState ===============================
		class TileMappingState {
			friend class StreamTexture;
		public:
			template<typename T> using TileRow = std::vector<T>;
			template<typename T> using TileMip = std::vector<TileRow<T>>;
			template<typename T> using TileSeq = std::vector<TileMip<T>>;

			/*
			* ����ÿ��Tile��פ��״̬
			*/
			enum class ResidencyState : uint8_t {
				NotResident = 0,
				Resident    = 1,
				Loading     = 2,
				Evicting    = 3 
			};
		public:
			TileMappingState(uint32_t mipNums, std::vector<D3D12_SUBRESOURCE_TILING>& subresourceTilings);
			~TileMappingState();

			void SetRefCount(uint32_t x, uint32_t y, uint32_t s, uint32_t refCount);
			void IncRefCount(uint32_t x, uint32_t y, uint32_t s);
			void DecRefCount(uint32_t x, uint32_t y, uint32_t s);
			void SetResidencyState(uint32_t x, uint32_t y, uint32_t s, ResidencyState state);
			void SetHeapAllocation(uint32_t x, uint32_t y, uint32_t s, BuddyHeapAllocator::Allocation* heapAllocation);

			inline const auto& GetRefCount(uint32_t x, uint32_t y, uint32_t s)       const { return mRefCounts[s][y][x]; }
			inline const auto& GetResidencyState(uint32_t x, uint32_t y, uint32_t s) const { return mResidencyStates[s][y][x]; }
			inline auto* GetHeapAllocation(uint32_t x, uint32_t y, uint32_t s) const { return mHeapAllocations[s][y][x]; }

		private:
			TileSeq<uint32_t> mRefCounts;								// ÿһ��Tile�����ü���
			TileSeq<ResidencyState> mResidencyStates;					// ÿһ��Tile��פ��״̬
			TileSeq<BuddyHeapAllocator::Allocation*> mHeapAllocations;	// ÿһ��Tile�ķ�����Ϣ
		};
		std::unique_ptr<TileMappingState> mTileMappingState;

		// =============================== Pending Information ===============================
		std::vector<D3D12_TILED_RESOURCE_COORDINATE> mPendingTileLoadings;

		/*
		* �����Tile��Loading������˵��Tile��Eviction������ǿ��ͬ�����⣬������������������ܷ����Դ��λ�Ӷ����²�������Ľ��
		* ��ˣ���������һ��Eviction��������ֻ����������N֡���ɼ�ʱ��Tile�Żᴥ��Eviction����
		*/
		class EvictionDelay {
		public:
			EvictionDelay(uint32_t nFrames = 4u);
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

		/*
		* ��һ��Tile������Eviction����ʱ����Tile��ӵ�е�HeapAllocation���ܱ������Ż��Դ���С�
		* ��Ϊ��HeapAllocation���ܻ�����һ��ѭ��ʱ������Tile���ã�������MinMipMap���ӳٸ������⣬���ʹ��GPU��Ȼ��Ϊ��HeapAllocation�����ѱ�Eviction��Tile������������������λ��
		* ��ˣ���HeapAllocation��Ҫ�ڱ���NumSwapBuffers��֮֡���ٱ��Żس����У��ò���ȷ����MinMipMap���ӳٸ��²�����ɺ����Ĳ�����λ���⡣
		*/
		class DeallocationDelay {
		public:

		private:
		};
	};

}