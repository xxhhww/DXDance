#pragma once
#include "DirectStorage/dstorage.h"

#include "Texture.h"
#include "XeTextureFormat.h"

#include "Tools/TemplateAllocator.h"

namespace GHL {
	class CommandQueue;
	class Fence;
}

namespace Renderer {

	class FileHandle;

	class StreamTexture {
	public:
		StreamTexture(
			const GHL::Device* device,
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

		void MapAndLoadPackedMipMap(GHL::CommandQueue* mappingQueue, GHL::Fence* mappingFence, IDStorageQueue* copyDsQueue, GHL::Fence* copyFence);

		void SetResidencyMapOffset(uint64_t mapOffset);

		inline auto* GetInternalResource() const { return mInternalTexture; }

		inline const auto& GetPackedMipInfo()   const { return mPackedMipInfo; }
		inline const auto& GetTileShape()       const { return mTileShape; }
		inline const auto& GetNumTilesTotal()   const { return mNumTilesTotal; }
		inline const auto& GetTilings()         const { return mTiling; }
		inline const auto& GetTileTexelWidth()  const { return mTileShape.WidthInTexels; }
		inline const auto& GetTileTexelHeight() const { return mTileShape.HeightInTexels; }
		inline const auto& GetTileTexelDepth()  const { return mTileShape.DepthInTexels; }
		inline const auto& GetNumTilesWidth()   const { return mTiling.at(0).WidthInTiles; }
		inline const auto& GetNumTilesHeight()  const { return mTiling.at(0).HeightInTiles; }
		inline const auto& GetNumTilesDepth()   const { return mTiling.at(0).DepthInTiles; }

		inline const auto& GetResidencyMapOffset() const { return mResidencyMapOffset; }

	private:
		const GHL::Device* mDevice { nullptr };
		XeTexureFormat mFileFormat;
		std::unique_ptr<FileHandle> mFileHandle;

		Texture* mInternalTexture { nullptr };
		RingFrameTracker* mFrameTracker { nullptr };
		BuddyHeapAllocator* mHeapAllocator { nullptr };

		D3D12_PACKED_MIP_INFO mPackedMipInfo; // last n mips may be packed into a single tile
		D3D12_TILE_SHAPE mTileShape;          // e.g. a 64K tile may contain 128x128 texels @ 4B/pixel
		UINT mNumTilesTotal;
		std::vector<D3D12_SUBRESOURCE_TILING> mTiling;

		uint32_t mMaxMip{ 0u }; // Start From 1u

		uint32_t mPackedMipsFileOffset{ 0u };
		uint32_t mPackedMipsNumBytes{ 0u };
		uint32_t mPackedMipsUncompressedSize{ 0u };
		BuddyHeapAllocator::Allocation* mPackedMipsHeapAllocation{ nullptr };

		Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> mClearUavHeap;
		D3D12_RESOURCE_DESC1 mFeedbackResourceDesc{};
		Microsoft::WRL::ComPtr<ID3D12Resource2> mFeedbackResource;
		DescriptorHandleWrap mFeedbackUADescriptor;

		Microsoft::WRL::ComPtr<ID3D12Resource> mResolvedResource;
		std::vector<Microsoft::WRL::ComPtr<ID3D12Resource>> mReadbackResource;

		std::vector<BYTE, AlignedAllocator<BYTE>> mMinMipMap; // 该纹理的MipLevel驻留信息，总是需要更新到GlobalMinMipMap中
		uint64_t mResidencyMapOffset{ 0u }; // 该纹理的驻留信息在全局驻留信息中的偏移量(索引)

	};

}