#pragma once
#include "Renderer/ReTextureFileFormat.h"
#include "Renderer/ResourceFormat.h"
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"

#include <DirectStorage/dstorage.h>

namespace Renderer {

	class TerrainRenderer;

	class TerrainTiledTexture {
	public:
		/*
		* ÃèÊöÃ¿¸öTileµÄ×¤Áô×´Ì¬
		*/
		enum class ResidencyState : uint8_t {
			NotResident = 0,
			Resident = 1,
			Loading = 2
		};

		class Tile {
		public:
			inline Tile() = default;
			inline ~Tile() = default;

		private:
			ResidencyState mResidencyState{ ResidencyState::NotResident };
			BuddyHeapAllocator::Allocation* mHeapAllocation{ nullptr };
		};

		class TileMappingState {
		public:
			template<typename T> using TileRow = std::vector<T>;
			template<typename T> using TileMip = std::vector<TileRow<T>>;
			template<typename T> using TileSeq = std::vector<TileMip<T>>;


			TileMappingState(const ReTextureFileFormat& reTextureFileFormat);
			~TileMappingState();
			
		private:
			TileSeq<Tile> mTileDatas;
		};

	public:
		TerrainTiledTexture(TerrainRenderer* renderer, const std::string& filepath);
		~TerrainTiledTexture();

		inline const auto& GetReTextureFileFormat() const { return mReTextureFileFormat; }
		inline const auto& GetResourceFormat() const { return mResourceFormat; }

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
		Microsoft::WRL::ComPtr<ID3D12Resource> mD3DResource{ nullptr };
		PoolDescriptorAllocator::Allocation* mDescriptorAllocation{ nullptr };

		std::unique_ptr<TileMappingState> mTileMappingStates;
	};

}