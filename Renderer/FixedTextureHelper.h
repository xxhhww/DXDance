#pragma once
#include "Renderer/ResourceAllocator.h"

namespace GHL {
	class Fence;
}

namespace Renderer {

	class FixedTextureHelper {
	public:
		static TextureWrap LoadFromFile(
			const GHL::Device* device,
			PoolDescriptorAllocator* descriptorAllocator,
			ResourceAllocator* resourceAllocator,
			IDStorageQueue* copyQueue,
			GHL::Fence* copyFence,
			const std::string& filepath);

		static void        SaveToFile  (
			const GHL::Device* device,
			TextureWrap& textureWrap, 
			BufferWrap&  rbBufferWrap,
			const std::string& filepath);
	};

}