#pragma once
#include "Renderer/ReTextureFileFormat.h"
#include "Renderer/ResourceFormat.h"
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"

#include <DirectStorage/dstorage.h>

namespace GHL {
	class Device;
}

namespace Renderer {

	class TerrainRenderer;

	/*
	* 管理地形渲染所使用的纹理数组，该纹理数组以保留形式创建
	*/
	class TerrainTextureArray {
	public:
		TerrainTextureArray(TerrainRenderer* renderer, const std::string& filepath);

		~TerrainTextureArray();

		inline const auto& GetNumTilesWidth()  const { return mTiling.at(0).WidthInTiles; }
		inline const auto& GetNumTilesHeight() const { return mTiling.at(0).HeightInTiles; }
		inline const auto& GetNumTilesDepth()  const { return mTiling.at(0).DepthInTiles; }

		inline const auto& GetReTextureFileFormat() const { return mReTextureFileFormat; }
		inline const auto& GetSubresourceFormat() const { return mResourceFormat; }

		inline auto* GetDStorageFile() const { return mDStorageFile.Get(); }

	private:
		void CreateD3DResource();

	private:
		TerrainRenderer* mRenderer{ nullptr };
		const GHL::Device* mDevice{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		ReTextureFileFormat mReTextureFileFormat;
		ResourceFormat mResourceFormat;
		Microsoft::WRL::ComPtr<IDStorageFile> mDStorageFile;

		Microsoft::WRL::ComPtr<ID3D12Resource> mD3DReservedResource;

		D3D12_PACKED_MIP_INFO mPackedMipInfo; // last n mips may be packed into a single tile
		D3D12_TILE_SHAPE mTileShape;          // e.g. a 64K tile may contain 128x128 texels @ 4B/pixel
		UINT mNumTilesTotal;
		std::vector<D3D12_SUBRESOURCE_TILING> mTiling;
		uint8_t mNumStandardMips{ 0u }; // Start From 1u
		uint8_t mNumStandardArrays{ 0u };
	};

}