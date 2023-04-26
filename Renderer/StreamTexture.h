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
		* 录制Feedback的清理命令
		*/
		void RecordClearFeedback(ID3D12GraphicsCommandList4* commandList);

		/*
		* 录制解算Feedback的命令
		*/
		void RecordResolve(ID3D12GraphicsCommandList4* commandList);

		/*
		* 录制回读ResolvedResource的命令
		*/
		void RecordReadback(ID3D12GraphicsCommandList4* commandList);

		/*
		* 映射并从文件中加载PackedMipMap数据，由渲染主线程直接调用，而非其他线程调用
		*/
		void MapAndLoadPackedMipMap(GHL::CommandQueue* mappingQueue, GHL::Fence* mappingFence, IDStorageQueue* copyDsQueue, GHL::Fence* copyFence);

		/*
		* 该函数并不在渲染主线程中运行，而是在ProcessFeedback线程中运行
		* 在每一个渲染帧完成后，渲染主线程会通知ProcessFeedback线程对Feedback进行处理，从而调用该函数
		* 因此，需要注意线程之间的同步问题
		*/
		void ProcessReadbackFeedback();

		/*
		* 处理Tile加载的任务
		*/
		void ProcessTileLoadings();

		/*
		* 处理Tile卸载的任务
		*/
		void ProcessTileEvictions();

		/*
		* 渲染帧完成后的回调函数，不直接注册到FrameTracker的渲染帧完成回调中，而是由StreamTextureManger中的同名回调函数调用
		*/
		void FrameCompletedCallback(uint8_t frameIndex);

		/*
		* 设置该StreamTexture的Residency在全局ResidencyMap中的偏移量
		*/
		void SetResidencyMapOffset(uint64_t mapOffset);

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
			uint64_t renderFrameFenceValue{ 0u };	// 该变量由渲染主线程写入，ProcessFeedback线程只读
			std::atomic<bool> isFresh{ false };		// 该变量由渲染主线程与ProcessFeedback线程进行访问与修改
		};
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mReadbackResource;
		/*
		* 在每一个渲染帧完成后，渲染主线程都会更新QueuedReadbackFeedback这个结构体对象中的isFresh变量，并通知ProcessFeedback线程进行处理
		* 而在ProcessFeedback线程中，也会对isFresh变量进行修改
		* 注意：渲染帧完成后的回调注册放在了StreamTextureStorage中，这样就不需要对每一个StreamTexture对象进行回调注册了
		*/
		std::vector<QueuedReadbackFeedback> mQueuedReadbackFeedback;

		// =============================== ResidencyMipMap ===============================
		std::vector<uint8_t> mMinMipMapCache;
		std::vector<BYTE, AlignedAllocator<BYTE>> mMinMipMap; // 该纹理的MipLevel驻留信息，总是需要更新到GlobalMinMipMap中
		uint64_t mResidencyMapOffset{ 0u }; // 该纹理的驻留信息在全局驻留信息中的偏移量(索引)

	private:
		// =============================== TileMappingState ===============================
		class TileMappingState {
			friend class StreamTexture;
		public:
			template<typename T> using TileRow = std::vector<T>;
			template<typename T> using TileMip = std::vector<TileRow<T>>;
			template<typename T> using TileSeq = std::vector<TileMip<T>>;

			/*
			* 描述每个Tile的驻留状态
			*/
			enum class ResidencyState : uint8_t {
				NotResident = 0, // b00
				Resident    = 1, // b01
				Evicting    = 2, // b10
				Loading     = 3, // b11
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
			inline const auto* GetHeapAllocation(uint32_t x, uint32_t y, uint32_t s) const { return mHeapAllocations[s][y][x]; }
			
		private:
			TileSeq<uint32_t> mRefCounts;								// 每一个Tile的引用计数
			TileSeq<ResidencyState> mResidencyStates;					// 每一个Tile的驻留状态
			TileSeq<BuddyHeapAllocator::Allocation*> mHeapAllocations;	// 每一个Tile的分配信息
		};
		std::unique_ptr<TileMappingState> mTileMappingState;

		// =============================== Pending Information ===============================
		std::vector<D3D12_TILED_RESOURCE_COORDINATE> mPendingLoadings;

		class EvictionDelay {
		public:
			EvictionDelay(UINT numSwapBuffers);
			~EvictionDelay() = default;

			using MappingCoords = std::vector<D3D12_TILED_RESOURCE_COORDINATE>;
			void Append(D3D12_TILED_RESOURCE_COORDINATE in_coord) { mMappings[0].push_back(in_coord); }
			MappingCoords& GetReadyToEvict() { return mMappings.back(); }

			void MoveToNextFrame();
			void Clear();

			// drop pending evictions for tiles that now have non-zero refcount
			void Rescue(const TileMappingState& in_tileMappingState);
		private:
			std::vector<MappingCoords> mMappings;
		};
		EvictionDelay mPendingEvictions;
	};

}