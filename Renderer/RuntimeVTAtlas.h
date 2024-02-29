#pragma once
#include "Renderer/ResourceAllocator.h"

namespace Renderer {

	class TerrainRenderer;

	class RuntimeVTAtlas {
	public:
		RuntimeVTAtlas(TerrainRenderer* terrainRenderer, DXGI_FORMAT dxgiFormat, const std::string& name);
		~RuntimeVTAtlas() = default;

		inline const auto& GetTileSizeNoPadding()   const { return mTileSizeNoPadding; }
		inline const auto& GetPaddingSize()         const { return mPaddingSize; }
		inline const auto& GetTileCountPerAxis()    const { return mTileCountPerAxis; }
		inline const auto& GetTileCount()           const { return mTileCount; }
		inline const auto& GetTextureAtlasSize()    const { return mTextureAtlasSize; }

		inline const auto  GetTileSizeWithPadding() const { return mTileSizeNoPadding + 2u * mPaddingSize; }

		inline const auto& GetDxgiFormat()          const { return mDxgiFormat; }

		inline auto& GetTextureAtlas() { return mTextureAtlas; }

	private:
		TerrainRenderer* mRenderer{ nullptr };
		DXGI_FORMAT mDxgiFormat;

		uint32_t mTileSizeNoPadding;	// Tile的大小
		uint32_t mPaddingSize;			// Tile的填充大小

		uint32_t mTileCountPerAxis;		// 物理纹理中每个轴的Tile的平铺个数
		uint32_t mTileCount;			// 物理纹理中Tile的总个数

		uint32_t mTextureAtlasSize;		// 纹理图集的尺寸大小

		TextureWrap mTextureAtlas;
	};

}