#pragma once
#include "Renderer/ResourceAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"

namespace Renderer {

	class GrasslandRenderer;

	class GrasslandLinearBuffer {
	public:
		GrasslandLinearBuffer(GrasslandRenderer* renderer, uint32_t tileCount, uint32_t bytesPerBalde, uint32_t bladesPerTile);
		inline ~GrasslandLinearBuffer() = default;

		inline const auto& GetTileCount()     const { return mTileCount; }
		inline const auto& GetBytesPerBlade() const { return mBytesPerBlade; }
		inline const auto& GetBladesPerTile() const { return mBladesPerTile; }
		inline const auto& GetBytesPerTile()  const { return mBytesPerTile; }

		inline auto& GetLinearBuffer() const { return mLinearBuffer; }

	private:
		GrasslandRenderer* mRenderer{ nullptr };

		uint32_t mTileCount;		// 切片个数
		uint32_t mBytesPerBlade;	// 每一个Blade的字节大小
		uint32_t mBladesPerTile;	// 每一个切片的草点个数
		uint32_t mBytesPerTile;		// 每一个切片的字节大小

		BufferWrap mLinearBuffer;	// 线性缓冲
	};

}