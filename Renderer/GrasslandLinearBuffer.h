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

		uint32_t mTileCount;		// ��Ƭ����
		uint32_t mBytesPerBlade;	// ÿһ��Blade���ֽڴ�С
		uint32_t mBladesPerTile;	// ÿһ����Ƭ�Ĳݵ����
		uint32_t mBytesPerTile;		// ÿһ����Ƭ���ֽڴ�С

		BufferWrap mLinearBuffer;	// ���Ի���
	};

}