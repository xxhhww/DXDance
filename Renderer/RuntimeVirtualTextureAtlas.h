#pragma once
#include "Renderer/ResourceAllocator.h"

namespace Renderer {

	class TerrainRenderer;

	class RuntimeVirtualTextureAtlas {
	public:
		RuntimeVirtualTextureAtlas(TerrainRenderer* terrainRenderer, DXGI_FORMAT dxgiFormat, const std::string& name);
		~RuntimeVirtualTextureAtlas() = default;

		inline const auto& GetTileSizeNoPadding()   const { return mTileSizeNoPadding; }
		inline const auto& GetPaddingSize()         const { return mPaddingSize; }
		inline const auto& GetTileCountPerAxis()    const { return mTileCountPerAxis; }
		inline const auto& GetTileCount()           const { return mTileCount; }
		inline const auto& GetTextureAtlasSize()    const { return mPhysicalTextureSize; }

		inline const auto  GetTileSizeWithPadding() const { return mTileSizeNoPadding + 2u * mPaddingSize; }

	private:
		TerrainRenderer* mRenderer{ nullptr };

		uint32_t mTileSizeNoPadding;	// Tile�Ĵ�С
		uint32_t mPaddingSize;			// Tile������С

		uint32_t mTileCountPerAxis;		// ����������ÿ�����Tile��ƽ�̸���
		uint32_t mTileCount;			// ����������Tile���ܸ���

		uint32_t mPhysicalTextureSize;	// ��������ĳߴ��С

		TextureWrap mPhysicalTexture;
	};

}