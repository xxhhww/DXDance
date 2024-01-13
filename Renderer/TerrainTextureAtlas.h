#pragma once
#include "Renderer/ReTextureFileFormat.h"
#include "Renderer/ResourceFormat.h"
#include "Renderer/BuddyHeapAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"
#include "Renderer/ResourceAllocator.h"

#include "GHL/DirectStorageFile.h"

namespace Renderer {

	class TerrainRenderer;
	class TerrainPhysicalTexture;

	class TerrainTextureAtlas {
	public:
		TerrainTextureAtlas(TerrainRenderer* renderer, const std::string& filepath, uint32_t tileCountPerAxis);
		inline ~TerrainTextureAtlas() = default;

		inline auto* GetDStorageFile() const { return mDStorageFile.get(); }
		inline auto& GetTextureAtlas() const { return mTextureAtlas; }

		inline const auto& GetReTextureFileFormat() const { return mReTextureFileFormat; }
		inline const auto& GetTileSize()            const { return mReTextureFileFormat.GetFileHeader().tileWidth; }
		inline const auto& GetTileCountPerAxis()    const { return mTileCountPerAxis; }
		inline const auto& GetTileCount()           const { return mTileCount; }
		inline const auto& GetTextureAtlasSize()    const { return mTextureAtlasSize; }

		inline const size_t GetUncompressedSize()   const { return mReTextureFileFormat.GetTileByteSize(); }

	private:
		TerrainRenderer* mRenderer{ nullptr };

		ReTextureFileFormat mReTextureFileFormat;

		std::unique_ptr<GHL::DirectStorageFile> mDStorageFile;

		uint32_t mTileCountPerAxis;				// ͼ����ÿ�����Tile��ƽ�̸���
		uint32_t mTileCount;					// ͼ����Tile���ܸ���

		uint32_t mTextureAtlasSize;			// ͼ�����ܳߴ��С

		TextureWrap mTextureAtlas;			// ͼ��D3D����
	};

}