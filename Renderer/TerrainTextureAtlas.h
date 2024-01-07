#pragma once
#include "Renderer/ReTextureFileFormat.h"
#include "Renderer/ResourceFormat.h"
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"

#include <DirectStorage/dstorage.h>

namespace Renderer {

	class TerrainRenderer;

	class TerrainTextureAtlas {
	public:
		class Tile {
		public:
			Tile(
				const GHL::Device* device,
				TerrainTextureAtlas* atlas,
				BuddyHeapAllocator* heapAllocator,
				PoolDescriptorAllocator* descriptorAllocator
			);
			~Tile();

			void Create();

			void Release();

		private:
			void CreateSRDescriptor(const TextureSubResourceDesc& subDesc = TextureSubResourceDesc{});

		private:
			const GHL::Device* mDevice{ nullptr };
			TerrainTextureAtlas* mTextureAtlas{ nullptr };
			BuddyHeapAllocator* mHeapAllocator{ nullptr };
			PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

			Microsoft::WRL::ComPtr<ID3D12Resource> mD3DResource{ nullptr };

			PoolDescriptorAllocator::Allocation* mDescriptorAllocation{ nullptr };
			BuddyHeapAllocator::Allocation* mHeapAllocation{ nullptr };
		};

	public:
		TerrainTextureAtlas(TerrainRenderer* renderer, const std::string& filepath);
		~TerrainTextureAtlas();

		inline const auto& GetReTextureFileFormat() const { return mReTextureFileFormat; }
		inline const auto& GetSubresourceFormat() const { return mSubResourceFormat; }
		
		inline auto* GetDStorageFile() const { return mDStorageFile.Get(); }

		Tile* GetTileData(uint8_t x, uint8_t y, uint8_t lod) const;

	private:
		TerrainRenderer* mRenderer{ nullptr };
		const GHL::Device* mDevice{ nullptr };
		BuddyHeapAllocator* mHeapAllocator{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };

		ReTextureFileFormat mReTextureFileFormat;
		ResourceFormat mSubResourceFormat;

		Microsoft::WRL::ComPtr<IDStorageFile> mDStorageFile;

		std::vector<Tile> mTileDatas;
	};

}