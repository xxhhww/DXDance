#pragma once
#include "Renderer/TileCache.h"
#include "Renderer/ResourceAllocator.h"

namespace Renderer {

	class RuntimeVirtualTextureSystem;

	class RvtPhysicalTexture {
	public:
		RvtPhysicalTexture(RuntimeVirtualTextureSystem* rvtSystem);
		~RvtPhysicalTexture() = default;

		inline auto& GetTileCache() { return mTileCache; }
		inline const auto& GetTileCache() const { return mTileCache; }

		inline const auto& GetTileCountPerAxis()    const { return mTileCountPerAxis; }
		inline const auto& GetTileSize()            const { return mTileSize; }
		inline const auto& GetPaddingSize()         const { return mPaddingSize; }
		inline const auto& GetTileCount()           const { return mTileCount; }
		inline const auto& GetPhysicalTextureSize() const { return mPhysicalTextureSize; }

		inline const auto  GetTileSizeWithPadding() const { return mTileSize + 2u * mPaddingSize; }

		inline auto& GetPhysicalTextures() { return mPhysicalTextures; }
		inline const auto& GetPhysicalTextures() const { return mPhysicalTextures; }

	private:
		RenderEngine* mRenderEngine{ nullptr };
		RuntimeVirtualTextureSystem* mRvtSystem{ nullptr };

		// From RvtSystem
		uint32_t mTileSize;
		uint32_t mPaddingSize;

		uint32_t mTileCountPerAxis;		// ����������ÿ�����Tile��ƽ�̸���
		uint32_t mTileCount;			// ����������Tile���ܸ���
		uint32_t mPhysicalTextureSize;	// ��������ĳߴ��С

		TileCache mTileCache;			// Tile LRU Cache

		std::vector<TextureWrap> mPhysicalTextures;
	};

}