#pragma once
#include "Renderer/ResourceAllocator.h"
#include "Renderer/PoolDescriptorAllocator.h"
#include "Renderer/TerrainRenderer.h"

namespace GHL {
	class Device;
}

namespace Renderer {

	class GrasslandLinearBuffer {
	public:

	private:
		TerrainRenderer* mRenderer{ nullptr };
		const GHL::Device* mDevice{ nullptr };
		PoolDescriptorAllocator* mDescriptorAllocator{ nullptr };
	};

}