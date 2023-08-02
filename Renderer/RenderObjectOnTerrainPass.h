#pragma once
#include "Renderer/RenderGraph.h"
#include "Renderer/ResourceAllocator.h"

namespace Renderer {

	class RenderEngine;

	class RenderObjectOnTerrainPass {
	public:
		struct RenderObjectOnTerrainPassData {
			uint32_t  grassPlacementBufferIndex;
			uint32_t  numVertices;	// LOD
			float pad1;
			float pad2;
		};

		RenderObjectOnTerrainPassData renderObjectOnTerrainPassData;
		inline static uint32_t lod0NumVertices = 9u;

	public:
		void AddPass(RenderGraph& renderGraph);
	};

}